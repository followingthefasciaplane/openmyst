#!/usr/bin/env python3
"""
audits assets against game.db
"""

import os
import sys
import sqlite3
import struct
import zipfile
import json
import glob as globmod
from collections import defaultdict
from pathlib import Path

# ─── Configuration ───────────────────────────────────────────────────────────

ASSETS_DIR = os.path.dirname(os.path.abspath(__file__))
CONTENT_DIR = os.path.join(ASSETS_DIR, "content")
MAPS_DIR = os.path.join(ASSETS_DIR, "maps")
SCRIPTS_DIR = os.path.join(ASSETS_DIR, "scripts")
DB_PATH = os.path.join(ASSETS_DIR, "game.db")

# Severity levels
CRITICAL = "CRITICAL"  # Will crash/break the client
MAJOR = "MAJOR"        # Visible glitches/missing content
MINOR = "MINOR"        # Naming inconsistencies, non-functional
INFO = "INFO"          # Informational findings


class Issue:
    def __init__(self, severity, category, description, details=None):
        self.severity = severity
        self.category = category
        self.description = description
        self.details = details or ""

    def __repr__(self):
        return f"[{self.severity}] {self.category}: {self.description}"


class AssetAuditor:
    def __init__(self):
        self.zip_index = {}          # {filename: [zip_path, ...]}
        self.zip_contents = {}       # {zip_path: [filenames]}
        self.disk_files = {}         # {relative_path: absolute_path}
        self.issues = []
        self.stats = defaultdict(int)

    # ─── Phase 1A: ZIP Content Indexer ────────────────────────────────────

    def index_zips(self):
        """Scan all ZIP files in content/ and build filename index."""
        print("=" * 70)
        print("PHASE 1A: Indexing ZIP archives")
        print("=" * 70)

        zip_files = []
        for root, dirs, files in os.walk(CONTENT_DIR):
            for f in files:
                if f.endswith(".zip"):
                    zip_files.append(os.path.join(root, f))
        zip_files.sort()

        total_files = 0
        for zpath in zip_files:
            rel_zip = os.path.relpath(zpath, CONTENT_DIR)
            try:
                with zipfile.ZipFile(zpath, 'r') as zf:
                    names = [n for n in zf.namelist() if not n.endswith('/')]
                    self.zip_contents[rel_zip] = names
                    for name in names:
                        basename = os.path.basename(name)
                        if basename not in self.zip_index:
                            self.zip_index[basename] = []
                        self.zip_index[basename].append(rel_zip)
                        total_files += 1
            except zipfile.BadZipFile:
                self.issues.append(Issue(CRITICAL, "ZIP", f"Corrupt ZIP file: {rel_zip}"))

        print(f"  Indexed {len(zip_files)} ZIP archives containing {total_files} files")
        print(f"  Unique filenames: {len(self.zip_index)}")
        self.stats["zip_count"] = len(zip_files)
        self.stats["zip_file_count"] = total_files

    # ─── Phase 1A.2: Disk file index ─────────────────────────────────────

    def index_disk_files(self):
        """Index all non-ZIP files on disk in assets/."""
        print("\n  Indexing disk files...")
        for root, dirs, files in os.walk(ASSETS_DIR):
            # Skip the content dir's ZIPs (already indexed)
            for f in files:
                if f.endswith(".zip"):
                    continue
                full = os.path.join(root, f)
                rel = os.path.relpath(full, ASSETS_DIR)
                self.disk_files[rel] = full
        print(f"  Indexed {len(self.disk_files)} disk files")

    # ─── Helper: check if asset exists ────────────────────────────────────

    def asset_exists(self, filename, expected_zips=None):
        """Check if a filename exists in any ZIP (or specific ZIPs).
        Returns (found, locations)."""
        if not filename or filename.strip() == "":
            return True, []  # Empty/null = no reference
        basename = os.path.basename(filename)
        locations = self.zip_index.get(basename, [])
        if expected_zips:
            # Check if it's in any of the expected locations
            matching = [z for z in locations if any(e in z for e in expected_zips)]
            return len(matching) > 0, locations
        return len(locations) > 0, locations

    def disk_file_exists(self, rel_path):
        """Check if a file exists on disk relative to assets/."""
        return rel_path in self.disk_files or os.path.exists(os.path.join(ASSETS_DIR, rel_path))

    # ─── Phase 1B: DB Asset Reference Checker ─────────────────────────────

    def check_db_assets(self):
        """Query game.db tables and verify referenced files exist."""
        print("\n" + "=" * 70)
        print("PHASE 1B: Checking database asset references")
        print("=" * 70)

        conn = sqlite3.connect(DB_PATH)
        conn.row_factory = sqlite3.Row

        self._check_item_template(conn)
        self._check_item_dictionary(conn)
        self._check_item_gems(conn)
        self._check_spell_template(conn)
        self._check_spell_visual_kit(conn)
        self._check_npc_models(conn)
        self._check_npc_template_portraits(conn)
        self._check_npc_sounds(conn)
        self._check_gameobject_models(conn)
        self._check_model_info(conn)
        self._check_map_audio(conn)
        self._check_zone_audio(conn)
        self._check_area_audio(conn)
        self._check_sprite_hotspot(conn)
        self._check_sprite_light(conn)
        self._check_sprite_proximity_sound(conn)
        self._check_sprite_psi(conn)

        conn.close()

    def _check_item_template(self, conn):
        print("\n  [item_template] Checking icons, sounds, models...")
        cur = conn.execute("SELECT entry, name, icon, icon_sound, model FROM item_template")
        missing_icons = []
        missing_sounds = []
        missing_models = []
        total = 0
        for row in cur:
            total += 1
            entry, name, icon, icon_sound, model = row["entry"], row["name"], row["icon"], row["icon_sound"], row["model"]

            if icon and icon.strip():
                found, locs = self.asset_exists(icon)
                if not found:
                    missing_icons.append((entry, name, icon))

            if icon_sound and icon_sound.strip():
                found, locs = self.asset_exists(icon_sound)
                if not found:
                    missing_sounds.append((entry, name, icon_sound))

            if model and model.strip() and str(model) != '0':
                # Item models are base names: look for player_male_<model>.png / player_female_<model>.png
                male_sprite = f"player_male_{model}.png"
                female_sprite = f"player_female_{model}.png"
                found_m, _ = self.asset_exists(male_sprite)
                found_f, _ = self.asset_exists(female_sprite)
                if not found_m and not found_f:
                    missing_models.append((entry, name, model))

        print(f"    Checked {total} items")
        if missing_icons:
            print(f"    MISSING icons: {len(missing_icons)}")
            for entry, name, icon in missing_icons:
                self.issues.append(Issue(MAJOR, "item_template.icon",
                    f"Item {entry} '{name}': icon '{icon}' not found in any ZIP"))
        if missing_sounds:
            print(f"    MISSING sounds: {len(missing_sounds)}")
            for entry, name, snd in missing_sounds:
                self.issues.append(Issue(MINOR, "item_template.icon_sound",
                    f"Item {entry} '{name}': sound '{snd}' not found in any ZIP"))
        if missing_models:
            unique_models = set(m[2] for m in missing_models)
            print(f"    MISSING models: {len(missing_models)} items referencing {len(unique_models)} unique models")
            # Only report unique model names to avoid massive duplication
            for mdl in sorted(unique_models):
                example = next(m for m in missing_models if m[2] == mdl)
                count = sum(1 for m in missing_models if m[2] == mdl)
                self.issues.append(Issue(MAJOR, "item_template.model",
                    f"Model '{mdl}' (player_male_{mdl}.png / player_female_{mdl}.png) not found ({count} items use it, e.g. item {example[0]} '{example[1]}')"))
        if not missing_icons and not missing_sounds and not missing_models:
            print("    All OK")

    def _check_item_dictionary(self, conn):
        print("\n  [item_dictionary] Checking icons, sounds, models...")
        cur = conn.execute("SELECT rowid, icon, sound, model FROM item_dictionary")
        missing = []
        total = 0
        for row in cur:
            total += 1
            for col in ["icon", "sound"]:
                val = row[col]
                if val and val.strip():
                    found, _ = self.asset_exists(val)
                    if not found:
                        missing.append((row["rowid"], col, val))
            # Models are base names like item_template
            model = row["model"]
            if model and model.strip():
                male_sprite = f"player_male_{model}.png"
                female_sprite = f"player_female_{model}.png"
                found_m, _ = self.asset_exists(male_sprite)
                found_f, _ = self.asset_exists(female_sprite)
                if not found_m and not found_f:
                    missing.append((row["rowid"], "model", model))
        print(f"    Checked {total} rows")
        for rid, col, val in missing:
            if col == "model":
                self.issues.append(Issue(MAJOR, f"item_dictionary.{col}",
                    f"Row {rid}: model '{val}' (player_male_{val}.png / player_female_{val}.png) not found"))
            else:
                self.issues.append(Issue(MAJOR, f"item_dictionary.{col}",
                    f"Row {rid}: '{val}' not found in any ZIP"))
        if not missing:
            print("    All OK")
        else:
            print(f"    MISSING: {len(missing)}")

    def _check_item_gems(self, conn):
        print("\n  [item_gems] Checking icons...")
        cur = conn.execute("SELECT gem_id, icon, name FROM item_gems")
        missing = []
        total = 0
        for row in cur:
            total += 1
            icon = row["icon"]
            if icon and icon.strip():
                found, _ = self.asset_exists(icon)
                if not found:
                    missing.append((row["gem_id"], row["name"], icon))
        print(f"    Checked {total} gems")
        for gid, name, icon in missing:
            self.issues.append(Issue(MAJOR, "item_gems.icon",
                f"Gem {gid} '{name}': icon '{icon}' not found"))
        if not missing:
            print("    All OK")
        else:
            print(f"    MISSING: {len(missing)}")

    def _check_spell_template(self, conn):
        print("\n  [spell_template] Checking icons...")
        cur = conn.execute("SELECT entry, name, icon FROM spell_template")
        missing = []
        in_item_icons = []
        total = 0
        for row in cur:
            total += 1
            icon = row["icon"]
            if icon and icon.strip():
                found_spell, _ = self.asset_exists(icon, ["spell_icons"])
                found_item, _ = self.asset_exists(icon, ["item_icons"])
                found_any, locs = self.asset_exists(icon)
                if not found_any:
                    missing.append((row["entry"], row["name"], icon))
                elif not found_spell and found_item:
                    in_item_icons.append((row["entry"], row["name"], icon))
        print(f"    Checked {total} spells")
        if missing:
            print(f"    MISSING icons: {len(missing)}")
            for eid, name, icon in missing:
                self.issues.append(Issue(MAJOR, "spell_template.icon",
                    f"Spell {eid} '{name}': icon '{icon}' not found"))
        if in_item_icons:
            print(f"    In item_icons.zip (not spell_icons.zip): {len(in_item_icons)}")
            for eid, name, icon in in_item_icons:
                self.issues.append(Issue(INFO, "spell_template.icon",
                    f"Spell {eid} '{name}': icon '{icon}' in item_icons.zip, not spell_icons.zip"))
        if not missing and not in_item_icons:
            print("    All OK")

    def _check_spell_visual_kit(self, conn):
        print("\n  [spell_visual_kit] Checking psystem, spranim, sounds...")
        cur = conn.execute("SELECT id, psystem, spranim, spranim_2, directional_spranim, sound FROM spell_visual_kit")
        missing = []
        total = 0
        for row in cur:
            total += 1
            svk_id = row["id"]
            for col in ["psystem", "spranim", "spranim_2", "directional_spranim", "sound"]:
                val = row[col]
                if val and val.strip() and val != '0':
                    # directional_spranim uses $d template pattern (e.g. arrow_$d.sa)
                    if col == "directional_spranim" and "$d" in val:
                        # Check that at least one direction variant exists
                        test_name = val.replace("$d", "east")
                        found_disk = self.disk_file_exists(f"scripts/animation/{test_name}")
                        found_zip, _ = self.asset_exists(test_name)
                        if not found_zip and not found_disk:
                            missing.append((svk_id, col, val, MAJOR))
                        continue

                    # Check both ZIP and disk
                    found_zip, _ = self.asset_exists(val)
                    found_disk = False
                    if col == "psystem":
                        found_disk = self.disk_file_exists(f"scripts/particles/{val}")
                        # Some psystem refs are actually .sa files in animation/
                        if not found_disk and val.endswith(".sa"):
                            found_disk = self.disk_file_exists(f"scripts/animation/{val}")
                    elif col in ("spranim", "spranim_2"):
                        found_disk = self.disk_file_exists(f"scripts/animation/{val}")
                    elif col == "sound":
                        found_zip, _ = self.asset_exists(val)

                    if not found_zip and not found_disk:
                        severity = CRITICAL if val.endswith(".saa") else MAJOR
                        missing.append((svk_id, col, val, severity))
        print(f"    Checked {total} visual kits")
        for sid, col, val, sev in missing:
            self.issues.append(Issue(sev, f"spell_visual_kit.{col}",
                f"SVK {sid}: '{val}' not found", "Typo?" if val.endswith(".saa") else ""))
        if not missing:
            print("    All OK")
        else:
            print(f"    MISSING: {len(missing)}")

    def _check_npc_models(self, conn):
        print("\n  [npc_models] Checking sprite files (npc_<name>.png)...")
        cur = conn.execute("SELECT id, name, height FROM npc_models")
        missing = []
        total = 0
        for row in cur:
            total += 1
            name = row["name"]
            if name and name.strip():
                sprite_name = f"npc_{name}.png"
                found, locs = self.asset_exists(sprite_name)
                if not found:
                    # Check for close matches
                    close = [k for k in self.zip_index if name in k and k.startswith("npc_")]
                    missing.append((row["id"], name, sprite_name, close))
        print(f"    Checked {total} models")
        if missing:
            print(f"    MISSING sprites: {len(missing)}")
            for mid, name, sprite, close in missing:
                detail = f"Close matches: {close}" if close else ""
                self.issues.append(Issue(MAJOR, "npc_models.sprite",
                    f"Model {mid} '{name}': expected '{sprite}' not found in any NPC ZIP", detail))
        else:
            print("    All OK")

    def _check_npc_template_portraits(self, conn):
        print("\n  [npc_template] Checking portraits (portrait_<name>.png)...")
        cur = conn.execute("SELECT entry, name, portrait FROM npc_template WHERE portrait IS NOT NULL AND portrait != ''")
        missing = []
        total = 0
        for row in cur:
            total += 1
            portrait = row["portrait"]
            if portrait and portrait.strip():
                portrait_file = f"portrait_{portrait}.png"
                found, locs = self.asset_exists(portrait_file)
                if not found:
                    missing.append((row["entry"], row["name"], portrait, portrait_file))
        print(f"    Checked {total} NPCs with portraits")
        if missing:
            unique_portraits = set(m[2] for m in missing)
            print(f"    MISSING portraits: {len(missing)} refs to {len(unique_portraits)} unique files")
            for entry, name, portrait, pfile in missing:
                self.issues.append(Issue(MAJOR, "npc_template.portrait",
                    f"NPC {entry} '{name}': portrait '{pfile}' not found"))
        else:
            print("    All OK")

    def _check_npc_sounds(self, conn):
        print("\n  [npc_sounds] Checking sound files...")
        cur = conn.execute("SELECT model, event, sound FROM npc_sounds")
        missing = []
        total = 0
        for row in cur:
            total += 1
            snd = row["sound"]
            if snd and snd.strip():
                found, _ = self.asset_exists(snd)
                if not found:
                    missing.append((row["model"], row["event"], snd))
        print(f"    Checked {total} NPC sounds")
        if missing:
            print(f"    MISSING sounds: {len(missing)}")
            for model, event, snd in missing:
                self.issues.append(Issue(MAJOR, "npc_sounds.sound",
                    f"NPC model '{model}' event '{event}': sound '{snd}' not found"))
        else:
            print("    All OK")

    def _check_gameobject_models(self, conn):
        print("\n  [gameobject_models] Checking textures, anims, sounds...")
        cur = conn.execute("""SELECT id, name_closed, name_open,
            anim_name_closed, anim_name_open, sound_use, sound_unlocked
            FROM gameobject_models""")
        missing = []
        total = 0
        for row in cur:
            total += 1
            goid = row["id"]
            for col in ["name_closed", "name_open"]:
                val = row[col]
                if val and val.strip():
                    found, _ = self.asset_exists(val)
                    if not found:
                        missing.append((goid, col, val))
            for col in ["anim_name_closed", "anim_name_open"]:
                val = row[col]
                if val and val.strip():
                    found_zip, _ = self.asset_exists(val)
                    found_disk = self.disk_file_exists(f"scripts/animation/{val}")
                    if not found_zip and not found_disk:
                        missing.append((goid, col, val))
            for col in ["sound_use", "sound_unlocked"]:
                val = row[col]
                if val and val.strip():
                    found, _ = self.asset_exists(val)
                    if not found:
                        missing.append((goid, col, val))
        print(f"    Checked {total} GO models")
        if missing:
            print(f"    MISSING: {len(missing)}")
            for goid, col, val in missing:
                self.issues.append(Issue(MAJOR, f"gameobject_models.{col}",
                    f"GO model {goid}: '{val}' not found"))
        else:
            print("    All OK")

    def _check_model_info(self, conn):
        print("\n  [model_info] Cross-referencing with npc_models...")
        mi_names = set()
        cur = conn.execute("SELECT DISTINCT script_name FROM model_info")
        for row in cur:
            mi_names.add(row["script_name"])

        npc_names = set()
        cur = conn.execute("SELECT DISTINCT name FROM npc_models")
        for row in cur:
            npc_names.add(row["name"])

        # model_info names not in npc_models
        in_mi_not_npc = mi_names - npc_names
        in_npc_not_mi = npc_names - mi_names

        print(f"    model_info unique names: {len(mi_names)}")
        print(f"    npc_models unique names: {len(npc_names)}")

        if in_mi_not_npc:
            print(f"    In model_info but NOT in npc_models: {len(in_mi_not_npc)}")
            for name in sorted(in_mi_not_npc):
                self.issues.append(Issue(MINOR, "model_info.cross_ref",
                    f"model_info script '{name}' has no matching npc_models entry"))
        if in_npc_not_mi:
            print(f"    In npc_models but NOT in model_info: {len(in_npc_not_mi)}")
            for name in sorted(in_npc_not_mi):
                self.issues.append(Issue(MINOR, "model_info.cross_ref",
                    f"npc_models name '{name}' has no model_info animation data"))

        if not in_mi_not_npc and not in_npc_not_mi:
            print("    All OK - perfect match")

    def _check_audio_refs(self, table, conn, label):
        """Generic audio checker for map/zone/area tables."""
        cur = conn.execute(f"SELECT * FROM {table}")
        missing = []
        total = 0
        for row in cur:
            total += 1
            row_id = row["id"]
            row_name = row["name"] if "name" in row.keys() else str(row_id)
            for col in ["music", "ambience"]:
                val = row[col]
                if val and val.strip():
                    # Handle comma-separated values
                    for track in val.split(","):
                        track = track.strip()
                        if not track:
                            continue
                        found, locs = self.asset_exists(track)
                        if not found:
                            missing.append((row_id, row_name, col, track))
        print(f"    Checked {total} {label}")
        if missing:
            print(f"    MISSING audio: {len(missing)}")
            for rid, rname, col, track in missing:
                self.issues.append(Issue(MAJOR, f"{table}.{col}",
                    f"{label.rstrip('s')} {rid} '{rname}': audio '{track}' not found"))
        else:
            print("    All OK")

    def _check_map_audio(self, conn):
        print(f"\n  [map] Checking music and ambience...")
        self._check_audio_refs("map", conn, "maps")

    def _check_zone_audio(self, conn):
        print(f"\n  [zone_template] Checking music and ambience...")
        self._check_audio_refs("zone_template", conn, "zones")

    def _check_area_audio(self, conn):
        print(f"\n  [area_template] Checking music and ambience...")
        self._check_audio_refs("area_template", conn, "areas")

    def _check_sprite_table(self, conn, table, file_col, extra_cols, expected_zips):
        """Generic sprite table checker."""
        cols = [file_col] + extra_cols
        col_str = ", ".join(cols)
        cur = conn.execute(f"SELECT rowid, {col_str} FROM {table}")
        missing = []
        total = 0
        for row in cur:
            total += 1
            # Check the filename column against map texture ZIPs and disk scripts
            fname = row[file_col]
            if fname and fname.strip():
                found, _ = self.asset_exists(fname)
                if not found:
                    # Also check disk for .psi/.sa files
                    found_disk = False
                    if fname.endswith(".psi"):
                        found_disk = self.disk_file_exists(f"scripts/particles/{fname}")
                    elif fname.endswith(".sa"):
                        found_disk = self.disk_file_exists(f"scripts/animation/{fname}")
                    if not found_disk:
                        missing.append((row["rowid"], file_col, fname))
            # Check extra columns (sound, psi refs)
            for col in extra_cols:
                val = row[col]
                if val and val.strip():
                    found_zip, _ = self.asset_exists(val)
                    found_disk = False
                    if val.endswith(".psi"):
                        found_disk = self.disk_file_exists(f"scripts/particles/{val}")
                    if not found_zip and not found_disk:
                        missing.append((row["rowid"], col, val))
        print(f"    Checked {total} rows")
        if missing:
            print(f"    MISSING: {len(missing)}")
            for rid, col, val in missing:
                self.issues.append(Issue(MAJOR, f"{table}.{col}",
                    f"Row {rid}: '{val}' not found"))
        else:
            print("    All OK")

    def _check_sprite_hotspot(self, conn):
        print(f"\n  [sprite_hotspot] Checking filenames...")
        self._check_sprite_table(conn, "sprite_hotspot", "filename", [],
            ["map_textures_", "map_tilesets"])

    def _check_sprite_light(self, conn):
        print(f"\n  [sprite_light] Checking filenames...")
        self._check_sprite_table(conn, "sprite_light", "filename", [],
            ["map_textures_", "map_tilesets"])

    def _check_sprite_proximity_sound(self, conn):
        print(f"\n  [sprite_proximity_sound] Checking filenames and sounds...")
        self._check_sprite_table(conn, "sprite_proximity_sound", "filename", ["sound"],
            ["map_textures_", "map_tilesets"])

    def _check_sprite_psi(self, conn):
        print(f"\n  [sprite_psi] Checking filenames and PSI refs...")
        self._check_sprite_table(conn, "sprite_psi", "filename", ["psi"],
            ["map_textures_", "map_tilesets"])

    # ─── Phase 1C: Script File References ─────────────────────────────────

    def check_script_files(self):
        """Check script references from known paths."""
        print("\n" + "=" * 70)
        print("PHASE 1C: Checking script file references")
        print("=" * 70)

        # Check known script paths that the binary references
        known_scripts = [
            "scripts/dbtables.txt",
            "scripts/dbtables_dev.txt",
            "scripts/sprite/portrait_offset.txt",
        ]
        for sp in known_scripts:
            if self.disk_file_exists(sp):
                print(f"  OK: {sp}")
            else:
                self.issues.append(Issue(MAJOR, "script_file",
                    f"Expected script file missing: {sp}"))
                print(f"  MISSING: {sp}")

        # Check bscript files reference PNGs that exist in interface.zip
        print("\n  Checking .bscript PNG/sound references...")
        bscript_dir = os.path.join(SCRIPTS_DIR, "buttons")
        if os.path.isdir(bscript_dir):
            missing_refs = []
            total_bscripts = 0
            for bfile in sorted(os.listdir(bscript_dir)):
                if not bfile.endswith(".bscript"):
                    continue
                total_bscripts += 1
                fpath = os.path.join(bscript_dir, bfile)
                with open(fpath, 'r', errors='replace') as f:
                    lines = [l.strip() for l in f.readlines() if l.strip() and not l.strip().startswith("#")]
                # Lines: name, idle.png, hover.png, press.png, [sound.ogg]
                for line in lines[1:]:  # Skip name line
                    if line.endswith(('.png', '.ogg', '.wav')):
                        found, _ = self.asset_exists(line)
                        if not found:
                            missing_refs.append((bfile, line))
            print(f"    Checked {total_bscripts} bscript files")
            if missing_refs:
                print(f"    MISSING refs: {len(missing_refs)}")
                for bfile, ref in missing_refs:
                    self.issues.append(Issue(MAJOR, "bscript.ref",
                        f"{bfile}: references '{ref}' not found in any ZIP"))
            else:
                print("    All OK")

        # Check particle script (.psi) files exist
        print("\n  Checking particle scripts...")
        psi_dir = os.path.join(SCRIPTS_DIR, "particles")
        if os.path.isdir(psi_dir):
            psi_files = [f for f in os.listdir(psi_dir) if f.endswith(".psi")]
            print(f"    Found {len(psi_files)} .psi files on disk")

        # Check animation scripts (.sa) files exist
        print("\n  Checking animation scripts...")
        anim_dir = os.path.join(SCRIPTS_DIR, "animation")
        if os.path.isdir(anim_dir):
            sa_files = [f for f in os.listdir(anim_dir) if f.endswith(".sa")]
            print(f"    Found {len(sa_files)} .sa files on disk")

        # Check shader files
        print("\n  Checking shader files...")
        shader_dir = os.path.join(SCRIPTS_DIR, "shaders")
        if os.path.isdir(shader_dir):
            shaders = os.listdir(shader_dir)
            print(f"    Found {len(shaders)} shader files: {', '.join(sorted(shaders))}")

    # ─── Phase 1D: DB Schema Validation ───────────────────────────────────

    def check_db_schema(self):
        """Validate FK relationships and data integrity in game.db."""
        print("\n" + "=" * 70)
        print("PHASE 1D: Database schema & FK validation")
        print("=" * 70)

        conn = sqlite3.connect(DB_PATH)

        # npc_template.model_id -> npc_models.id
        print("\n  Checking npc_template.model_id -> npc_models.id...")
        # Check for empty/null model_ids
        cur = conn.execute("""
            SELECT entry, name, model_id FROM npc_template
            WHERE model_id IS NULL OR model_id = '' OR model_id = 0
        """)
        empty_models = cur.fetchall()
        if empty_models:
            print(f"    EMPTY model_id: {len(empty_models)} npc_templates have no model_id set")
            for entry, name, mid in empty_models:
                self.issues.append(Issue(MAJOR, "FK.npc_template.model_id",
                    f"NPC {entry} '{name}': model_id is empty/null - needs a valid npc_models reference"))

        # Check for invalid model_ids
        cur = conn.execute("""
            SELECT nt.entry, nt.name, nt.model_id
            FROM npc_template nt
            LEFT JOIN npc_models nm ON nt.model_id = nm.id
            WHERE nm.id IS NULL AND nt.model_id IS NOT NULL
            AND nt.model_id != '' AND nt.model_id != 0
        """)
        orphans = cur.fetchall()
        if orphans:
            print(f"    BROKEN FK: {len(orphans)} npc_templates reference non-existent npc_models")
            for entry, name, mid in orphans:
                self.issues.append(Issue(CRITICAL, "FK.npc_template.model_id",
                    f"NPC {entry} '{name}': model_id={mid} does not exist in npc_models"))
        if not empty_models and not orphans:
            print("    All OK")

        # spell_template.entry referenced by npc_template spell columns
        print("\n  Checking npc_template spell references -> spell_template.entry...")
        spell_cols = ["spell_primary", "spell_1_id", "spell_2_id", "spell_3_id", "spell_4_id"]
        broken = []
        for col in spell_cols:
            cur = conn.execute(f"""
                SELECT nt.entry, nt.name, nt.{col}
                FROM npc_template nt
                LEFT JOIN spell_template st ON nt.{col} = st.entry
                WHERE st.entry IS NULL AND nt.{col} IS NOT NULL AND nt.{col} != 0
                AND nt.{col} != '' AND typeof(nt.{col}) != 'text'
            """)
            for row in cur:
                broken.append((row[0], row[1], col, row[2]))
        if broken:
            print(f"    BROKEN: {len(broken)} spell references")
            for entry, name, col, sid in broken:
                self.issues.append(Issue(MAJOR, f"FK.npc_template.{col}",
                    f"NPC {entry} '{name}': {col}={sid} not in spell_template"))
        else:
            print("    All OK")

        # item_template spell references
        print("\n  Checking item_template spell references -> spell_template.entry...")
        broken = []
        for i in range(1, 6):
            col = f"spell_{i}"
            cur = conn.execute(f"""
                SELECT it.entry, it.name, it.{col}
                FROM item_template it
                LEFT JOIN spell_template st ON it.{col} = st.entry
                WHERE st.entry IS NULL AND it.{col} IS NOT NULL AND it.{col} != 0
                AND it.{col} != '' AND typeof(it.{col}) != 'text'
            """)
            for row in cur:
                broken.append((row[0], row[1], col, row[2]))
        if broken:
            print(f"    BROKEN: {len(broken)} spell references")
            for entry, name, col, sid in broken:
                self.issues.append(Issue(MAJOR, f"FK.item_template.{col}",
                    f"Item {entry} '{name}': {col}={sid} not in spell_template"))
        else:
            print("    All OK")

        # npc_sounds.model -> npc_models.name
        print("\n  Checking npc_sounds.model -> npc_models.name...")
        cur = conn.execute("""
            SELECT ns.model, ns.event, ns.sound
            FROM npc_sounds ns
            LEFT JOIN npc_models nm ON ns.model = nm.name
            WHERE nm.name IS NULL
        """)
        orphans = cur.fetchall()
        if orphans:
            print(f"    BROKEN: {len(orphans)} npc_sounds reference non-existent npc_models names")
            for model, event, sound in orphans:
                self.issues.append(Issue(MINOR, "FK.npc_sounds.model",
                    f"npc_sounds model='{model}' event='{event}': no matching npc_models.name"))
        else:
            print("    All OK")

        # gameobject_template.model -> gameobject_models.id
        print("\n  Checking gameobject_template.model -> gameobject_models.id...")
        cur = conn.execute("""
            SELECT gt.entry, gt.name, gt.model
            FROM gameobject_template gt
            LEFT JOIN gameobject_models gm ON gt.model = gm.id
            WHERE gm.id IS NULL AND gt.model IS NOT NULL AND gt.model != 0
        """)
        orphans = cur.fetchall()
        if orphans:
            print(f"    BROKEN: {len(orphans)} GO templates reference non-existent models")
            for entry, name, mid in orphans:
                self.issues.append(Issue(MAJOR, "FK.gameobject_template.model",
                    f"GO template {entry} '{name}': model={mid} not in gameobject_models"))
        else:
            print("    All OK")

        conn.close()

    # ─── Phase 1E: Map File Texture Validation ───────────────────────────

    def check_map_textures(self):
        """Parse .map files and verify all referenced textures exist in ZIPs."""
        print("\n" + "=" * 70)
        print("PHASE 1E: Map file texture validation")
        print("=" * 70)

        map_files = sorted(globmod.glob(os.path.join(MAPS_DIR, "*.map")))
        print(f"  Found {len(map_files)} map files")

        all_missing = {}
        total_textures = 0

        for mpath in map_files:
            mname = os.path.basename(mpath)
            try:
                with open(mpath, 'rb') as f:
                    map_width = struct.unpack('<I', f.read(4))[0]
                    num_obj_tex = struct.unpack('<I', f.read(4))[0]

                    textures = []
                    for _ in range(num_obj_tex):
                        name = b''
                        while True:
                            ch = f.read(1)
                            if ch == b'\x00' or ch == b'':
                                break
                            name += ch
                        textures.append(name.decode('utf-8', errors='replace'))

                total_textures += len(textures)
                missing_in_map = []
                for tex in textures:
                    found, _ = self.asset_exists(tex)
                    if not found:
                        # Also check disk for .psi/.sa files (scripts)
                        found_disk = False
                        if tex.endswith(".psi"):
                            found_disk = self.disk_file_exists(f"scripts/particles/{tex}")
                        elif tex.endswith(".sa"):
                            found_disk = self.disk_file_exists(f"scripts/animation/{tex}")
                        if not found_disk:
                            missing_in_map.append(tex)

                if missing_in_map:
                    all_missing[mname] = missing_in_map
                    print(f"  {mname}: {len(textures)} textures, {len(missing_in_map)} MISSING")
                else:
                    if len(textures) > 0:
                        print(f"  {mname}: {len(textures)} textures - All OK")
                    else:
                        print(f"  {mname}: no object textures")

            except Exception as e:
                self.issues.append(Issue(CRITICAL, "map_file",
                    f"Failed to parse {mname}: {e}"))
                print(f"  {mname}: PARSE ERROR - {e}")

        print(f"\n  Total textures across all maps: {total_textures}")

        for mname, missing_list in all_missing.items():
            for tex in missing_list:
                self.issues.append(Issue(MAJOR, "map.texture",
                    f"Map '{mname}': texture '{tex}' not found in any ZIP"))

    # ─── Report Generation ────────────────────────────────────────────────

    def generate_report(self):
        """Generate final audit report."""
        print("\n" + "=" * 70)
        print("AUDIT REPORT")
        print("=" * 70)

        by_severity = defaultdict(list)
        by_category = defaultdict(list)
        for issue in self.issues:
            by_severity[issue.severity].append(issue)
            by_category[issue.category].append(issue)

        print(f"\nTotal issues found: {len(self.issues)}")
        for sev in [CRITICAL, MAJOR, MINOR, INFO]:
            count = len(by_severity.get(sev, []))
            if count:
                print(f"  {sev}: {count}")

        # Print by severity
        for sev in [CRITICAL, MAJOR, MINOR, INFO]:
            issues = by_severity.get(sev, [])
            if not issues:
                continue
            print(f"\n{'─' * 70}")
            print(f"  {sev} Issues ({len(issues)})")
            print(f"{'─' * 70}")
            for issue in issues:
                print(f"  [{issue.category}] {issue.description}")
                if issue.details:
                    print(f"    → {issue.details}")

        # Summary by category
        print(f"\n{'─' * 70}")
        print(f"  Issues by Category")
        print(f"{'─' * 70}")
        for cat in sorted(by_category.keys()):
            issues = by_category[cat]
            sevs = defaultdict(int)
            for i in issues:
                sevs[i.severity] += 1
            sev_str = ", ".join(f"{s}: {c}" for s, c in sorted(sevs.items()))
            print(f"  {cat}: {len(issues)} ({sev_str})")

        # Write JSON report
        report_path = os.path.join(ASSETS_DIR, "audit_report.json")
        report = {
            "summary": {
                "total_issues": len(self.issues),
                "by_severity": {s: len(v) for s, v in by_severity.items()},
                "zip_count": self.stats["zip_count"],
                "zip_file_count": self.stats["zip_file_count"],
            },
            "issues": [
                {
                    "severity": i.severity,
                    "category": i.category,
                    "description": i.description,
                    "details": i.details
                }
                for i in self.issues
            ]
        }
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"\nJSON report written to: {report_path}")

        return len(by_severity.get(CRITICAL, []))

    # ─── Main ─────────────────────────────────────────────────────────────

    def run(self):
        print("Dreadmyst Asset Audit")
        print(f"Assets dir: {ASSETS_DIR}")
        print(f"Content dir: {CONTENT_DIR}")
        print(f"Database: {DB_PATH}")
        print()

        self.index_zips()
        self.index_disk_files()
        self.check_db_assets()
        self.check_script_files()
        self.check_db_schema()
        self.check_map_textures()
        critical_count = self.generate_report()

        print(f"\nAudit complete. Critical issues: {critical_count}")
        return critical_count


if __name__ == "__main__":
    auditor = AssetAuditor()
    sys.exit(auditor.run())

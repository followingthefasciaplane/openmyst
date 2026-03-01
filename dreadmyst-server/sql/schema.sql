-- Dreadmyst Server MySQL Schema
-- Derived from exact SQL queries found in binary at verified addresses
-- All column names verified via Ghidra MCP string extraction

-- ============================================================
-- Auth
-- ============================================================
CREATE TABLE IF NOT EXISTS `auth_tokens` (
    `id`         INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `user_id`    INT UNSIGNED NOT NULL,
    `token`      VARCHAR(128) NOT NULL,
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_token` (`token`),
    INDEX `idx_created` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Accounts
-- ============================================================
CREATE TABLE IF NOT EXISTS `accounts` (
    `id`         INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `username`   VARCHAR(64) NOT NULL UNIQUE,
    `password`   VARCHAR(256) NOT NULL,
    `email`      VARCHAR(128) DEFAULT NULL,
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `last_login` DATETIME DEFAULT NULL,
    `is_gm`      TINYINT(1) NOT NULL DEFAULT 0,
    `is_admin`   TINYINT(1) NOT NULL DEFAULT 0,
    `banned`     TINYINT(1) NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `banned_accounts` (
    `account_id`   INT UNSIGNED NOT NULL,
    `banned_by`    INT UNSIGNED NOT NULL DEFAULT 0,
    `reason`       VARCHAR(256) DEFAULT '',
    `ban_date`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `unban_date`   DATETIME DEFAULT NULL,
    PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player (character) table
-- Column order matches binary SQL at 0x00587638:
-- guid, account, name, class, gender, portrait, level,
-- hp_pct, mp_pct, xp, money, position_x, position_y,
-- orientation, map, home_map, home_x, home_y,
-- logout_time, last_death_time, pvp_points, pk_count,
-- pvp_flag, progression, num_invested_spells,
-- combat_rating, time_played, is_gm, is_admin
-- ============================================================
CREATE TABLE IF NOT EXISTS `player` (
    `guid`                INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `account`             INT UNSIGNED NOT NULL,
    `name`                VARCHAR(32) NOT NULL UNIQUE,
    `class`               TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `gender`              TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `portrait`            TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `level`               SMALLINT UNSIGNED NOT NULL DEFAULT 1,
    `hp_pct`              FLOAT NOT NULL DEFAULT 1.0,
    `mp_pct`              FLOAT NOT NULL DEFAULT 1.0,
    `xp`                  INT UNSIGNED NOT NULL DEFAULT 0,
    `money`               INT UNSIGNED NOT NULL DEFAULT 0,
    `position_x`          FLOAT NOT NULL DEFAULT 0.0,
    `position_y`          FLOAT NOT NULL DEFAULT 0.0,
    `orientation`         FLOAT NOT NULL DEFAULT 0.0,
    `map`                 INT UNSIGNED NOT NULL DEFAULT 0,
    `home_map`            INT UNSIGNED NOT NULL DEFAULT 0,
    `home_x`              FLOAT NOT NULL DEFAULT 0.0,
    `home_y`              FLOAT NOT NULL DEFAULT 0.0,
    `logout_time`         BIGINT NOT NULL DEFAULT 0,
    `last_death_time`     BIGINT NOT NULL DEFAULT 0,
    `pvp_points`          INT NOT NULL DEFAULT 0,
    `pk_count`            INT NOT NULL DEFAULT 0,
    `pvp_flag`            TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `progression`         INT UNSIGNED NOT NULL DEFAULT 0,
    `num_invested_spells` INT UNSIGNED NOT NULL DEFAULT 0,
    `combat_rating`       INT UNSIGNED NOT NULL DEFAULT 0,
    `time_played`         INT UNSIGNED NOT NULL DEFAULT 0,
    `is_gm`               TINYINT(1) NOT NULL DEFAULT 0,
    `is_admin`            TINYINT(1) NOT NULL DEFAULT 0,
    INDEX `idx_account` (`account`),
    INDEX `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player inventory
-- Binary: INSERT INTO player_inventory (guid, slot, entry, affix,
--         affix_score, gem1, gem2, gem3, gem4, enchant_level,
--         count, soulbound, durability)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_inventory` (
    `guid`          INT UNSIGNED NOT NULL,
    `slot`          INT UNSIGNED NOT NULL,
    `entry`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix_score`   FLOAT NOT NULL DEFAULT 0.0,
    `gem1`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem2`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem3`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem4`          INT UNSIGNED NOT NULL DEFAULT 0,
    `enchant_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `count`         INT UNSIGNED NOT NULL DEFAULT 1,
    `soulbound`     TINYINT(1) NOT NULL DEFAULT 0,
    `durability`    INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `slot`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player equipment (same column structure as inventory)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_equipment` (
    `guid`          INT UNSIGNED NOT NULL,
    `slot`          INT UNSIGNED NOT NULL,
    `entry`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix_score`   FLOAT NOT NULL DEFAULT 0.0,
    `gem1`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem2`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem3`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem4`          INT UNSIGNED NOT NULL DEFAULT 0,
    `enchant_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `count`         INT UNSIGNED NOT NULL DEFAULT 1,
    `soulbound`     TINYINT(1) NOT NULL DEFAULT 0,
    `durability`    INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `slot`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player bank (same column structure)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_bank` (
    `guid`          INT UNSIGNED NOT NULL,
    `slot`          INT UNSIGNED NOT NULL,
    `entry`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix_score`   FLOAT NOT NULL DEFAULT 0.0,
    `gem1`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem2`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem3`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem4`          INT UNSIGNED NOT NULL DEFAULT 0,
    `enchant_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `count`         INT UNSIGNED NOT NULL DEFAULT 1,
    `soulbound`     TINYINT(1) NOT NULL DEFAULT 0,
    `durability`    INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `slot`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player spells
-- Binary: INSERT INTO player_spell (guid, spell_id, level)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_spell` (
    `guid`     INT UNSIGNED NOT NULL,
    `spell_id` INT UNSIGNED NOT NULL,
    `level`    INT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`guid`, `spell_id`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player spell cooldowns
-- Binary: INSERT INTO player_spell_cooldown (guid, spell_id,
--         start_date, end_date)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_spell_cooldown` (
    `guid`       INT UNSIGNED NOT NULL,
    `spell_id`   INT UNSIGNED NOT NULL,
    `start_date` BIGINT NOT NULL DEFAULT 0,
    `end_date`   BIGINT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spell_id`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player auras (saved buffs/debuffs)
-- Column names EXACTLY from binary INSERT at 0x00586ab8:
-- Note: "miliseconds" is a typo in the original binary
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_aura` (
    `guid`                  INT UNSIGNED NOT NULL,
    `caster_guid`           INT UNSIGNED NOT NULL DEFAULT 0,
    `spell_id`              INT UNSIGNED NOT NULL,
    `miliseconds`           INT NOT NULL DEFAULT 0,
    `m_stackCount`          INT NOT NULL DEFAULT 0,
    `m_spellLevel`          INT NOT NULL DEFAULT 0,
    `m_casterLevel`         INT NOT NULL DEFAULT 0,
    -- Effect 1 tick data
    `m_tickTotal_1`         FLOAT NOT NULL DEFAULT 0,
    `m_tickAmount_1`        FLOAT NOT NULL DEFAULT 0,
    `m_tickAmountTicked_1`  FLOAT NOT NULL DEFAULT 0,
    `m_casterGuid_1`        INT UNSIGNED NOT NULL DEFAULT 0,
    `m_numTicks_1`          INT NOT NULL DEFAULT 0,
    `m_numTicksCounter_1`   INT NOT NULL DEFAULT 0,
    `m_tickTimer_1`         INT NOT NULL DEFAULT 0,
    `m_tickIntervalMs_1`    INT NOT NULL DEFAULT 0,
    -- Effect 2 tick data
    `m_tickTotal_2`         FLOAT NOT NULL DEFAULT 0,
    `m_tickAmount_2`        FLOAT NOT NULL DEFAULT 0,
    `m_tickAmountTicked_2`  FLOAT NOT NULL DEFAULT 0,
    `m_casterGuid_2`        INT UNSIGNED NOT NULL DEFAULT 0,
    `m_numTicks_2`          INT NOT NULL DEFAULT 0,
    `m_numTicksCounter_2`   INT NOT NULL DEFAULT 0,
    `m_tickTimer_2`         INT NOT NULL DEFAULT 0,
    `m_tickIntervalMs_2`    INT NOT NULL DEFAULT 0,
    -- Effect 3 tick data
    `m_tickTotal_3`         FLOAT NOT NULL DEFAULT 0,
    `m_tickAmount_3`        FLOAT NOT NULL DEFAULT 0,
    `m_tickAmountTicked_3`  FLOAT NOT NULL DEFAULT 0,
    `m_casterGuid_3`        INT UNSIGNED NOT NULL DEFAULT 0,
    `m_numTicks_3`          INT NOT NULL DEFAULT 0,
    `m_numTicksCounter_3`   INT NOT NULL DEFAULT 0,
    `m_tickTimer_3`         INT NOT NULL DEFAULT 0,
    `m_tickIntervalMs_3`    INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `spell_id`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player stat investments
-- Binary: INSERT INTO player_stat_invest (guid, stat_type, amount)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_stat_invest` (
    `guid`      INT UNSIGNED NOT NULL,
    `stat_type` INT UNSIGNED NOT NULL,
    `amount`    INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `stat_type`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player waypoints (discovered waypoints)
-- Binary: INSERT INTO player_waypoints (guid, object_guid)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_waypoints` (
    `guid`        INT UNSIGNED NOT NULL,
    `object_guid` INT UNSIGNED NOT NULL,
    PRIMARY KEY (`guid`, `object_guid`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player mail
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_mail` (
    `id`            INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `guid`          INT UNSIGNED NOT NULL,
    `sender_guid`   INT UNSIGNED NOT NULL DEFAULT 0,
    `entry`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix`         INT UNSIGNED NOT NULL DEFAULT 0,
    `affix_score`   FLOAT NOT NULL DEFAULT 0.0,
    `gem1`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem2`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem3`          INT UNSIGNED NOT NULL DEFAULT 0,
    `gem4`          INT UNSIGNED NOT NULL DEFAULT 0,
    `enchant_level` INT UNSIGNED NOT NULL DEFAULT 0,
    `count`         INT UNSIGNED NOT NULL DEFAULT 1,
    `soulbound`     TINYINT(1) NOT NULL DEFAULT 0,
    `durability`    INT NOT NULL DEFAULT 0,
    `sent_date`     BIGINT NOT NULL DEFAULT 0,
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Player quest status
-- Binary: INSERT INTO player_quest_status (guid, quest_entry,
--         status, obj1_count, obj2_count, obj3_count, obj4_count,
--         rewarded)
-- ============================================================
CREATE TABLE IF NOT EXISTS `player_quest_status` (
    `guid`        INT UNSIGNED NOT NULL,
    `quest_entry` INT UNSIGNED NOT NULL,
    `status`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `obj1_count`  INT UNSIGNED NOT NULL DEFAULT 0,
    `obj2_count`  INT UNSIGNED NOT NULL DEFAULT 0,
    `obj3_count`  INT UNSIGNED NOT NULL DEFAULT 0,
    `obj4_count`  INT UNSIGNED NOT NULL DEFAULT 0,
    `rewarded`    TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`, `quest_entry`),
    INDEX `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Guilds
-- Binary SQL verified at 0x0057fce0, 0x0057fd50, etc.
-- ============================================================
CREATE TABLE IF NOT EXISTS `guild` (
    `id`          INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `name`        VARCHAR(64) NOT NULL UNIQUE,
    `leader_guid` INT UNSIGNED NOT NULL,
    `motd`        VARCHAR(256) NOT NULL DEFAULT '',
    `create_date` BIGINT NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `guild_member` (
    `guid` INT UNSIGNED NOT NULL,
    `id`   INT UNSIGNED NOT NULL,
    `rank` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`),
    INDEX `idx_guild` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- NPC groups (verified from binary at 0x00586298)
-- SELECT guid_leader, guid_member, distance, angle, ai,
--        linked_respawn, linked_loot FROM npc_groups
-- ============================================================
CREATE TABLE IF NOT EXISTS `npc_groups` (
    `guid_leader`    INT UNSIGNED NOT NULL,
    `guid_member`    INT UNSIGNED NOT NULL,
    `distance`       FLOAT NOT NULL DEFAULT 0.0,
    `angle`          FLOAT NOT NULL DEFAULT 0.0,
    `ai`             INT UNSIGNED NOT NULL DEFAULT 0,
    `linked_respawn` TINYINT(1) NOT NULL DEFAULT 0,
    `linked_loot`    TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid_leader`, `guid_member`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Model info (from binary at 0x00586010)
-- SELECT script_name, animation_id, duration_in_seconds FROM model_info
-- ============================================================
CREATE TABLE IF NOT EXISTS `model_info` (
    `id`                   INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `script_name`          VARCHAR(64) NOT NULL DEFAULT '',
    `animation_id`         INT UNSIGNED NOT NULL DEFAULT 0,
    `duration_in_seconds`  FLOAT NOT NULL DEFAULT 0.0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================
-- Server logs
-- ============================================================
CREATE TABLE IF NOT EXISTS `logs` (
    `id`         INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `timestamp`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `level`      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `category`   VARCHAR(32) DEFAULT '',
    `message`    TEXT,
    `account_id` INT UNSIGNED DEFAULT NULL,
    `player_guid` INT UNSIGNED DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

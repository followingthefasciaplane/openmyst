#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Core type aliases matching the original binary's data sizes (x86 32-bit)
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using Guid   = uint32;

// Server constants from binary analysis
constexpr int DREADMYST_REVISION        = 1188;
constexpr int DEFAULT_TCP_PORT          = 16383;
constexpr int TICK_INTERVAL_MS          = 100;     // 10 ticks/second
constexpr int MINUTE_TICK_MS            = 60000;
constexpr int TEN_MINUTE_TICK_MS        = 600000;
constexpr int AUTH_TIMEOUT_SECONDS      = 5;
constexpr int AUTH_TOKEN_VALIDITY_SECS  = 60;
constexpr int MAX_ACCEPTS_PER_TICK      = 99;
constexpr int MAX_INVENTORY_SLOTS       = 40;
constexpr int MAX_BANK_SLOTS            = 40;
constexpr int MAX_EQUIPMENT_SLOTS       = 12;
constexpr int MAX_SPELL_EFFECTS         = 3;
constexpr int MAX_AURA_EFFECTS          = 3;
constexpr int MAX_QUEST_OBJECTIVES      = 4;
constexpr int MAX_QUEST_REWARDS         = 4;
constexpr int MAX_QUEST_REWARD_CHOICES  = 4;
constexpr int MAX_QUEST_PREREQS         = 3;
constexpr int MAX_NPC_SPELLS            = 4;
constexpr int MAX_ITEM_STATS            = 10;
constexpr int MAX_ITEM_SPELLS           = 5;
constexpr int MAX_ITEM_SOCKETS          = 4;
constexpr int MAX_AFFIX_STATS           = 5;
constexpr int MAX_GOSSIP_CONDITIONS     = 3;
constexpr int MAX_LOOT_CONDITIONS       = 2;
constexpr int MAX_GAMEOBJ_DATA          = 11;

// Session states (from binary at 0x00422c90)
enum class SessionState : uint8 {
    Disconnected   = 0,
    Connected      = 1,
    Authenticating = 2,
    Authenticated  = 4
};

// Player stats
enum class StatType : uint8 {
    Strength     = 0,
    Agility      = 1,
    Willpower    = 2,
    Intelligence = 3,
    Courage      = 4,
    Hp           = 5,
    Mana         = 6,
    Count        = 7
};

// Item quality tiers
enum class ItemQuality : uint8 {
    White  = 0,
    Green  = 1,
    Blue   = 2,
    Gold   = 3,
    Purple = 4
};

// Item equip types
enum class EquipType : uint8 {
    None       = 0,
    Head       = 1,
    Chest      = 2,
    Legs       = 3,
    Feet       = 4,
    Hands      = 5,
    Shoulders  = 6,
    Belt       = 7,
    Ring1      = 8,
    Ring2      = 9,
    Necklace   = 10,
    MainHand   = 11,
    OffHand    = 12
};

// NPC AI types
enum class AiType : uint8 {
    Melee        = 0,
    CasterArcher = 1
};

// NPC flags
enum NpcFlags : uint32 {
    NPC_FLAG_NONE       = 0x00,
    NPC_FLAG_GOSSIP     = 0x01,
    NPC_FLAG_VENDOR     = 0x02,
    NPC_FLAG_QUEST      = 0x04,
    NPC_FLAG_REPAIR     = 0x08,
    NPC_FLAG_BANKER     = 0x10,
    NPC_FLAG_INNKEEPER  = 0x20
};

// Game object types
enum class GameObjType : uint8 {
    NullType    = 0,
    Container   = 1,
    MapChanger  = 2,
    Waypoint    = 3
};

// Movement types
enum class MovementType : uint8 {
    Idle    = 0,
    Random  = 1,
    Patrol  = 2,
    Home    = 3,
    Chase   = 4,
    Charge  = 5,
    Point   = 6,
    Fear    = 7,
    Slide   = 8,
    Action  = 9
};

// Script commands
enum class ScriptCommand : uint8 {
    Talk        = 0,
    CastSpell   = 1,
    KillCredit  = 2,
    LocateNpc   = 3,
    OpenBank    = 4,
    PromptRespec = 5,
    QueueArena  = 6
};

// Spell effect types (from Effect_* class hierarchy)
enum class SpellEffectType : uint32 {
    NullEffect       = 0,
    SchoolDamage     = 1,
    WeaponDamage     = 2,
    MeleeAtk         = 3,
    RangedAtk        = 4,
    Heal             = 5,
    HealPct          = 6,
    HealthDrain      = 7,
    ManaDrain        = 8,
    ManaBurn         = 9,
    RestoreMana      = 10,
    RestoreManaPct   = 11,
    ApplyAura        = 12,
    ApplyAreaAura    = 13,
    Charge           = 14,
    KnockBack        = 15,
    PullTo           = 16,
    SlideFrom        = 17,
    Teleport         = 18,
    TeleportForward  = 19,
    LearnSpell       = 20,
    TriggerSpell     = 21,
    CreateItem       = 22,
    CombineItem      = 23,
    DestroyGems      = 24,
    ExtractOrb       = 25,
    LootEffect       = 26,
    Dispel           = 27,
    Duel             = 28,
    Gossip           = 29,
    Inspect          = 30,
    InterruptCast    = 31,
    Kill             = 32,
    SummonNpc        = 33,
    SummonObject     = 34,
    Resurrect        = 35,
    Threat           = 36,
    NearestWayp      = 37,
    ScriptEffect     = 38,
    ApplyGemSocket   = 39,
    ApplyOrbEnchant  = 40
};

// Aura effect types (from AuraEffect_* class hierarchy)
enum class AuraEffectType : uint32 {
    NullAuraType           = 0,
    ModifyStat             = 1,
    ModifyStatPct          = 2,
    ModifyDamageDealtPct   = 3,
    ModifyDamageReceivedPct = 4,
    ModifyHealingDealtPct  = 5,
    ModifyHealingRecvPct   = 6,
    ModifyMoveSpeedPct     = 7,
    AbsorbDamage           = 8,
    PeriodicDamage         = 9,
    PeriodicMeleeDamage    = 10,
    PeriodicHeal           = 11,
    PeriodicHealPct        = 12,
    PeriodicRestoreMana    = 13,
    PeriodicRestoreManaPct = 14,
    PeriodicBurnMana       = 15,
    InflictMechanic        = 16,
    MechanicImmune         = 17,
    SchoolImmunity         = 18,
    Proc                   = 19,
    Model                  = 20
};

// Condition types for gossip/loot
enum class ConditionType : uint32 {
    None         = 0,
    QuestDone    = 1,
    QuestActive  = 2,
    HasItem      = 3,
    Level        = 4,
    Class        = 5
};

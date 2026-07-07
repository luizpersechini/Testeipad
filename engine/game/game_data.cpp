/*
 * Command & Conquer: Tiberian Dawn - Modern Port
 * Game Data Registry Implementation
 *
 * Initializes all unit, building, and infantry type data.
 * Values ported from the original UDATA.CPP, BDATA.CPP, IDATA.CPP.
 */

#include "game_types.h"

namespace CnC {

static GameDataRegistry* g_registry = nullptr;

GameDataRegistry& GameDataRegistry::instance() {
    if (!g_registry) {
        g_registry = new GameDataRegistry();
    }
    return *g_registry;
}

void GameDataRegistry::init() {
    init_units();
    init_buildings();
    init_infantry();
}

const UnitTypeData& GameDataRegistry::get_unit(UnitType type) const {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(units_.size()))
        return units_[idx];
    static UnitTypeData null_data;
    return null_data;
}

const BuildingTypeData& GameDataRegistry::get_building(BuildingType type) const {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(buildings_.size()))
        return buildings_[idx];
    static BuildingTypeData null_data;
    return null_data;
}

const InfantryTypeData& GameDataRegistry::get_infantry(InfantryType type) const {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(infantry_.size()))
        return infantry_[idx];
    static InfantryTypeData null_data;
    return null_data;
}

void GameDataRegistry::init_units() {
    units_.resize(static_cast<int>(UnitType::Count));

    // ---- Mammoth Tank (HTANK) ----
    auto& mammoth = units_[static_cast<int>(UnitType::HTANK)];
    mammoth.type = UnitType::HTANK;
    mammoth.name = "Mammoth Tank";
    mammoth.sprite_base = "unit_mammoth";
    mammoth.owner = HouseType::GDI;
    mammoth.hit_points = 600;
    mammoth.cost = 1500;
    mammoth.build_time = 180;
    mammoth.sight_range = 5;
    mammoth.speed = 3;
    mammoth.speed_type = SpeedType::Track;
    mammoth.armor = ArmorType::Steel;
    mammoth.can_crush_infantry = true;
    mammoth.tech_level = 10;
    mammoth.primary_weapon = {WeaponType::MammothTusk, 60, 6, 50, "proj_missile", "snd_rocket"};
    mammoth.secondary_weapon = {WeaponType::TurretGun, 40, 5, 40, "proj_bullet", "snd_cannon"};
    mammoth.prerequisites = {BuildingType::WEAP, BuildingType::EYE};

    // ---- Medium Tank (MTANK) ----
    auto& medium = units_[static_cast<int>(UnitType::MTANK)];
    medium.type = UnitType::MTANK;
    medium.name = "Medium Tank";
    medium.sprite_base = "unit_medium_tank";
    medium.owner = HouseType::GDI;
    medium.hit_points = 400;
    medium.cost = 800;
    medium.build_time = 120;
    medium.sight_range = 4;
    medium.speed = 5;
    medium.speed_type = SpeedType::Track;
    medium.armor = ArmorType::Steel;
    medium.can_crush_infantry = true;
    medium.tech_level = 5;
    medium.primary_weapon = {WeaponType::TurretGun, 30, 5, 40, "proj_bullet", "snd_cannon"};
    medium.prerequisites = {BuildingType::WEAP};

    // ---- Light Tank (LTANK) ----
    auto& light = units_[static_cast<int>(UnitType::LTANK)];
    light.type = UnitType::LTANK;
    light.name = "Light Tank";
    light.sprite_base = "unit_light_tank";
    light.owner = HouseType::Nod;
    light.hit_points = 300;
    light.cost = 600;
    light.build_time = 90;
    light.sight_range = 4;
    light.speed = 6;
    light.speed_type = SpeedType::Track;
    light.armor = ArmorType::Steel;
    light.can_crush_infantry = true;
    light.tech_level = 3;
    light.primary_weapon = {WeaponType::TurretGun, 25, 4, 35, "proj_bullet", "snd_cannon"};
    light.prerequisites = {BuildingType::WEAP};

    // ---- Stealth Tank (STANK) ----
    auto& stealth = units_[static_cast<int>(UnitType::STANK)];
    stealth.type = UnitType::STANK;
    stealth.name = "Stealth Tank";
    stealth.sprite_base = "unit_stealth_tank";
    stealth.owner = HouseType::Nod;
    stealth.hit_points = 180;
    stealth.cost = 900;
    stealth.build_time = 150;
    stealth.sight_range = 4;
    stealth.speed = 7;
    stealth.speed_type = SpeedType::Track;
    stealth.armor = ArmorType::Aluminum;
    stealth.can_cloak = true;
    stealth.tech_level = 7;
    stealth.primary_weapon = {WeaponType::Rocket, 40, 5, 60, "proj_missile", "snd_rocket"};
    stealth.prerequisites = {BuildingType::WEAP, BuildingType::TMPL};

    // ---- APC ----
    auto& apc = units_[static_cast<int>(UnitType::APC)];
    apc.type = UnitType::APC;
    apc.name = "APC";
    apc.sprite_base = "unit_apc";
    apc.owner = HouseType::GDI;
    apc.hit_points = 200;
    apc.cost = 700;
    apc.build_time = 100;
    apc.sight_range = 4;
    apc.speed = 6;
    apc.speed_type = SpeedType::Track;
    apc.armor = ArmorType::Steel;
    apc.passengers = 5;
    apc.can_crush_infantry = true;
    apc.tech_level = 3;
    apc.primary_weapon = {WeaponType::Machinegun, 15, 4, 20, "proj_bullet", "snd_machinegun"};
    apc.prerequisites = {BuildingType::WEAP};

    // ---- MLRS ----
    auto& mlrs = units_[static_cast<int>(UnitType::MLRS)];
    mlrs.type = UnitType::MLRS;
    mlrs.name = "MLRS";
    mlrs.sprite_base = "unit_mlrs";
    mlrs.owner = HouseType::GDI;
    mlrs.hit_points = 150;
    mlrs.cost = 800;
    mlrs.build_time = 120;
    mlrs.sight_range = 5;
    mlrs.speed = 5;
    mlrs.speed_type = SpeedType::Track;
    mlrs.armor = ArmorType::Aluminum;
    mlrs.tech_level = 6;
    mlrs.primary_weapon = {WeaponType::Rocket, 50, 7, 60, "proj_missile", "snd_rocket"};
    mlrs.prerequisites = {BuildingType::WEAP, BuildingType::EYE};

    // ---- Rocket Launcher (MSAM) ----
    auto& msam = units_[static_cast<int>(UnitType::MSAM)];
    msam.type = UnitType::MSAM;
    msam.name = "Rocket Launcher";
    msam.sprite_base = "unit_rocket_launcher";
    msam.owner = HouseType::Nod;
    msam.hit_points = 150;
    msam.cost = 450;
    msam.build_time = 90;
    msam.sight_range = 5;
    msam.speed = 5;
    msam.speed_type = SpeedType::Track;
    msam.armor = ArmorType::Aluminum;
    msam.tech_level = 5;
    msam.primary_weapon = {WeaponType::Rocket, 40, 6, 50, "proj_missile", "snd_rocket"};
    msam.prerequisites = {BuildingType::WEAP};

    // ---- Harvester ----
    auto& harv = units_[static_cast<int>(UnitType::HARV)];
    harv.type = UnitType::HARV;
    harv.name = "Harvester";
    harv.sprite_base = "unit_harvester";
    harv.owner = HouseType::None; // Both factions
    harv.hit_points = 600;
    harv.cost = 1400;
    harv.build_time = 150;
    harv.sight_range = 3;
    harv.speed = 4;
    harv.speed_type = SpeedType::Track;
    harv.armor = ArmorType::Steel;
    harv.is_harvester = true;
    harv.can_crush_infantry = true;
    harv.tech_level = 1;
    harv.prerequisites = {BuildingType::PROC};

    // ---- MCV ----
    auto& mcv = units_[static_cast<int>(UnitType::MCV)];
    mcv.type = UnitType::MCV;
    mcv.name = "MCV";
    mcv.sprite_base = "unit_mcv";
    mcv.owner = HouseType::None;
    mcv.hit_points = 600;
    mcv.cost = 5000;
    mcv.build_time = 300;
    mcv.sight_range = 3;
    mcv.speed = 3;
    mcv.speed_type = SpeedType::Track;
    mcv.armor = ArmorType::Steel;
    mcv.can_crush_infantry = true;
    mcv.tech_level = 10;
    mcv.prerequisites = {BuildingType::WEAP};

    // ---- Humvee ----
    auto& humvee = units_[static_cast<int>(UnitType::JEEP)];
    humvee.type = UnitType::JEEP;
    humvee.name = "Humvee";
    humvee.sprite_base = "unit_humvee";
    humvee.owner = HouseType::GDI;
    humvee.hit_points = 150;
    humvee.cost = 400;
    humvee.build_time = 60;
    humvee.sight_range = 4;
    humvee.speed = 8;
    humvee.speed_type = SpeedType::Wheel;
    humvee.armor = ArmorType::Aluminum;
    humvee.tech_level = 2;
    humvee.primary_weapon = {WeaponType::Machinegun, 15, 4, 15, "proj_bullet", "snd_machinegun"};
    humvee.prerequisites = {BuildingType::WEAP};

    // ---- Nod Buggy ----
    auto& buggy = units_[static_cast<int>(UnitType::BUGGY)];
    buggy.type = UnitType::BUGGY;
    buggy.name = "Nod Buggy";
    buggy.sprite_base = "unit_buggy";
    buggy.owner = HouseType::Nod;
    buggy.hit_points = 140;
    buggy.cost = 300;
    buggy.build_time = 50;
    buggy.sight_range = 4;
    buggy.speed = 9;
    buggy.speed_type = SpeedType::Wheel;
    buggy.armor = ArmorType::Aluminum;
    buggy.tech_level = 1;
    buggy.primary_weapon = {WeaponType::Machinegun, 12, 4, 15, "proj_bullet", "snd_machinegun"};
    buggy.prerequisites = {BuildingType::WEAP};

    // ---- Recon Bike ----
    auto& bike = units_[static_cast<int>(UnitType::BIKE)];
    bike.type = UnitType::BIKE;
    bike.name = "Recon Bike";
    bike.sprite_base = "unit_recon_bike";
    bike.owner = HouseType::Nod;
    bike.hit_points = 120;
    bike.cost = 500;
    bike.build_time = 70;
    bike.sight_range = 5;
    bike.speed = 10;
    bike.speed_type = SpeedType::Wheel;
    bike.armor = ArmorType::None;
    bike.tech_level = 3;
    bike.primary_weapon = {WeaponType::Rocket, 30, 5, 50, "proj_missile", "snd_rocket"};
    bike.prerequisites = {BuildingType::WEAP};

    // ---- Artillery ----
    auto& arty = units_[static_cast<int>(UnitType::ARTY)];
    arty.type = UnitType::ARTY;
    arty.name = "Artillery";
    arty.sprite_base = "unit_artillery";
    arty.owner = HouseType::Nod;
    arty.hit_points = 100;
    arty.cost = 450;
    arty.build_time = 80;
    arty.sight_range = 6;
    arty.speed = 3;
    arty.speed_type = SpeedType::Track;
    arty.armor = ArmorType::Aluminum;
    arty.tech_level = 5;
    arty.primary_weapon = {WeaponType::Artillery, 60, 8, 80, "proj_shell", "snd_artillery"};
    arty.prerequisites = {BuildingType::WEAP};

    // ---- SSM Launcher ----
    auto& ssm = units_[static_cast<int>(UnitType::STNK)];
    ssm.type = UnitType::STNK;
    ssm.name = "SSM Launcher";
    ssm.sprite_base = "unit_ssm";
    ssm.owner = HouseType::Nod;
    ssm.hit_points = 120;
    ssm.cost = 750;
    ssm.build_time = 120;
    ssm.sight_range = 5;
    ssm.speed = 4;
    ssm.speed_type = SpeedType::Track;
    ssm.armor = ArmorType::Aluminum;
    ssm.tech_level = 7;
    ssm.primary_weapon = {WeaponType::SSM, 80, 9, 100, "proj_missile", "snd_rocket"};
    ssm.prerequisites = {BuildingType::WEAP, BuildingType::TMPL};

    // ---- Flame Tank ----
    auto& flame = units_[static_cast<int>(UnitType::FTNK)];
    flame.type = UnitType::FTNK;
    flame.name = "Flame Tank";
    flame.sprite_base = "unit_flame_tank";
    flame.owner = HouseType::Nod;
    flame.hit_points = 300;
    flame.cost = 800;
    flame.build_time = 110;
    flame.sight_range = 3;
    flame.speed = 5;
    flame.speed_type = SpeedType::Track;
    flame.armor = ArmorType::Steel;
    flame.can_crush_infantry = true;
    flame.tech_level = 5;
    flame.primary_weapon = {WeaponType::Flamethrower, 50, 3, 30, "proj_flame", "snd_flame"};
    flame.prerequisites = {BuildingType::WEAP};

    // ---- Gunboat ----
    auto& boat = units_[static_cast<int>(UnitType::BOAT)];
    boat.type = UnitType::BOAT;
    boat.name = "Gunboat";
    boat.sprite_base = "unit_gunboat";
    boat.owner = HouseType::GDI;
    boat.hit_points = 700;
    boat.cost = 0; // Not buildable
    boat.sight_range = 5;
    boat.speed = 4;
    boat.speed_type = SpeedType::Float;
    boat.armor = ArmorType::Steel;
    boat.tech_level = 99;
    boat.primary_weapon = {WeaponType::TurretGun, 40, 6, 50, "proj_bullet", "snd_cannon"};

    // ---- Hovercraft ----
    auto& hover = units_[static_cast<int>(UnitType::HOVER)];
    hover.type = UnitType::HOVER;
    hover.name = "Hovercraft";
    hover.sprite_base = "unit_hovercraft";
    hover.owner = HouseType::GDI;
    hover.hit_points = 400;
    hover.cost = 0; // Not buildable
    hover.sight_range = 4;
    hover.speed = 6;
    hover.speed_type = SpeedType::Hover;
    hover.armor = ArmorType::Steel;
    hover.passengers = 5;
    hover.tech_level = 99;
}

void GameDataRegistry::init_buildings() {
    buildings_.resize(static_cast<int>(BuildingType::Count));

    // ---- Construction Yard (HQ) ----
    auto& cy = buildings_[static_cast<int>(BuildingType::HQ)];
    cy.type = BuildingType::HQ;
    cy.name = "Construction Yard";
    cy.sprite_base = "bld_construction_yard";
    cy.hit_points = 1000;
    cy.cost = 5000;
    cy.build_time = 0;
    cy.sight_range = 5;
    cy.power_output = 0;
    cy.armor = ArmorType::Concrete;
    cy.width_cells = 3;
    cy.height_cells = 3;
    cy.has_bib = true;
    cy.tech_level = 1;

    // ---- Power Plant (NUKE) ----
    auto& pp = buildings_[static_cast<int>(BuildingType::NUKE)];
    pp.type = BuildingType::NUKE;
    pp.name = "Power Plant";
    pp.sprite_base = "bld_power_plant";
    pp.hit_points = 400;
    pp.cost = 300;
    pp.build_time = 60;
    pp.sight_range = 3;
    pp.power_output = 100;
    pp.armor = ArmorType::Wood;
    pp.width_cells = 2;
    pp.height_cells = 2;
    pp.has_bib = true;
    pp.tech_level = 1;

    // ---- Advanced Power Plant (NUK2) ----
    auto& app = buildings_[static_cast<int>(BuildingType::NUK2)];
    app.type = BuildingType::NUK2;
    app.name = "Advanced Power Plant";
    app.sprite_base = "bld_adv_power_plant";
    app.hit_points = 600;
    app.cost = 700;
    app.build_time = 90;
    app.sight_range = 3;
    app.power_output = 200;
    app.armor = ArmorType::Concrete;
    app.width_cells = 2;
    app.height_cells = 2;
    app.has_bib = true;
    app.tech_level = 5;
    app.prerequisites = {BuildingType::NUKE};

    // ---- Tiberium Refinery (PROC) ----
    auto& ref = buildings_[static_cast<int>(BuildingType::PROC)];
    ref.type = BuildingType::PROC;
    ref.name = "Tiberium Refinery";
    ref.sprite_base = "bld_refinery";
    ref.hit_points = 450;
    ref.cost = 2000;
    ref.build_time = 120;
    ref.sight_range = 4;
    ref.power_output = -30;
    ref.armor = ArmorType::Wood;
    ref.width_cells = 3;
    ref.height_cells = 2;
    ref.has_bib = true;
    ref.tech_level = 1;
    ref.prerequisites = {BuildingType::NUKE};

    // ---- Tiberium Silo (SILO) ----
    auto& silo = buildings_[static_cast<int>(BuildingType::SILO)];
    silo.type = BuildingType::SILO;
    silo.name = "Tiberium Silo";
    silo.sprite_base = "bld_silo";
    silo.hit_points = 150;
    silo.cost = 150;
    silo.build_time = 30;
    silo.sight_range = 2;
    silo.power_output = 0;
    silo.armor = ArmorType::Wood;
    silo.width_cells = 1;
    silo.height_cells = 1;
    silo.tech_level = 1;
    silo.prerequisites = {BuildingType::PROC};

    // ---- Barracks (FACT) ----
    auto& barracks = buildings_[static_cast<int>(BuildingType::FACT)];
    barracks.type = BuildingType::FACT;
    barracks.name = "Barracks";
    barracks.sprite_base = "bld_barracks";
    barracks.owner = HouseType::GDI;
    barracks.hit_points = 400;
    barracks.cost = 300;
    barracks.build_time = 60;
    barracks.sight_range = 3;
    barracks.power_output = -10;
    barracks.armor = ArmorType::Wood;
    barracks.width_cells = 2;
    barracks.height_cells = 2;
    barracks.has_bib = true;
    barracks.produces_infantry = true;
    barracks.tech_level = 1;
    barracks.prerequisites = {BuildingType::NUKE};

    // ---- Hand of Nod (HAND) ----
    auto& hand = buildings_[static_cast<int>(BuildingType::HAND)];
    hand.type = BuildingType::HAND;
    hand.name = "Hand of Nod";
    hand.sprite_base = "bld_hand_of_nod";
    hand.owner = HouseType::Nod;
    hand.hit_points = 400;
    hand.cost = 300;
    hand.build_time = 60;
    hand.sight_range = 3;
    hand.power_output = -10;
    hand.armor = ArmorType::Wood;
    hand.width_cells = 2;
    hand.height_cells = 2;
    hand.has_bib = true;
    hand.produces_infantry = true;
    hand.tech_level = 1;
    hand.prerequisites = {BuildingType::NUKE};

    // ---- Weapons Factory (WEAP) ----
    auto& wf = buildings_[static_cast<int>(BuildingType::WEAP)];
    wf.type = BuildingType::WEAP;
    wf.name = "Weapons Factory";
    wf.sprite_base = "bld_weapons_factory";
    wf.hit_points = 500;
    wf.cost = 2000;
    wf.build_time = 150;
    wf.sight_range = 3;
    wf.power_output = -30;
    wf.armor = ArmorType::Concrete;
    wf.width_cells = 3;
    wf.height_cells = 2;
    wf.has_bib = true;
    wf.produces_units = true;
    wf.tech_level = 2;
    wf.prerequisites = {BuildingType::NUKE, BuildingType::PROC};

    // ---- Guard Tower (GTWR) ----
    auto& gt = buildings_[static_cast<int>(BuildingType::GTWR)];
    gt.type = BuildingType::GTWR;
    gt.name = "Guard Tower";
    gt.sprite_base = "bld_guard_tower";
    gt.owner = HouseType::GDI;
    gt.hit_points = 300;
    gt.cost = 500;
    gt.build_time = 60;
    gt.sight_range = 5;
    gt.power_output = -10;
    gt.armor = ArmorType::Concrete;
    gt.width_cells = 1;
    gt.height_cells = 1;
    gt.tech_level = 2;
    gt.weapon = {WeaponType::Machinegun, 20, 5, 30, "proj_bullet", "snd_machinegun"};
    gt.prerequisites = {BuildingType::FACT};

    // ---- Advanced Guard Tower (ATWR) ----
    auto& agt = buildings_[static_cast<int>(BuildingType::ATWR)];
    agt.type = BuildingType::ATWR;
    agt.name = "Advanced Guard Tower";
    agt.sprite_base = "bld_adv_guard_tower";
    agt.owner = HouseType::GDI;
    agt.hit_points = 500;
    agt.cost = 1000;
    agt.build_time = 90;
    agt.sight_range = 6;
    agt.power_output = -20;
    agt.armor = ArmorType::Concrete;
    agt.width_cells = 1;
    agt.height_cells = 2;
    agt.tech_level = 6;
    agt.weapon = {WeaponType::Rocket, 40, 6, 50, "proj_missile", "snd_rocket"};
    agt.prerequisites = {BuildingType::GTWR, BuildingType::EYE};

    // ---- Turret (GUN) ----
    auto& turret = buildings_[static_cast<int>(BuildingType::GUN)];
    turret.type = BuildingType::GUN;
    turret.name = "Turret";
    turret.sprite_base = "bld_turret";
    turret.owner = HouseType::Nod;
    turret.hit_points = 200;
    turret.cost = 600;
    turret.build_time = 60;
    turret.sight_range = 5;
    turret.power_output = -20;
    turret.armor = ArmorType::Concrete;
    turret.width_cells = 1;
    turret.height_cells = 1;
    turret.tech_level = 3;
    turret.weapon = {WeaponType::TurretGun, 35, 5, 40, "proj_bullet", "snd_cannon"};
    turret.prerequisites = {BuildingType::WEAP};

    // ---- Obelisk of Light (OBELISK) ----
    auto& obelisk = buildings_[static_cast<int>(BuildingType::OBELISK)];
    obelisk.type = BuildingType::OBELISK;
    obelisk.name = "Obelisk of Light";
    obelisk.sprite_base = "bld_obelisk";
    obelisk.owner = HouseType::Nod;
    obelisk.hit_points = 300;
    obelisk.cost = 1500;
    obelisk.build_time = 120;
    obelisk.sight_range = 6;
    obelisk.power_output = -150;
    obelisk.armor = ArmorType::Concrete;
    obelisk.width_cells = 1;
    obelisk.height_cells = 2;
    obelisk.tech_level = 7;
    obelisk.weapon = {WeaponType::Obelisk, 200, 7, 120, "proj_laser", "snd_obelisk"};
    obelisk.prerequisites = {BuildingType::WEAP, BuildingType::TMPL};

    // ---- SAM Site (SAM) ----
    auto& sam = buildings_[static_cast<int>(BuildingType::SAM)];
    sam.type = BuildingType::SAM;
    sam.name = "SAM Site";
    sam.sprite_base = "bld_sam_site";
    sam.owner = HouseType::Nod;
    sam.hit_points = 200;
    sam.cost = 750;
    sam.build_time = 60;
    sam.sight_range = 5;
    sam.power_output = -20;
    sam.armor = ArmorType::Concrete;
    sam.width_cells = 2;
    sam.height_cells = 1;
    sam.tech_level = 4;
    sam.weapon = {WeaponType::Rocket, 50, 7, 60, "proj_missile", "snd_rocket"};
    sam.prerequisites = {BuildingType::WEAP};

    // ---- Temple of Nod (TMPL) ----
    auto& temple = buildings_[static_cast<int>(BuildingType::TMPL)];
    temple.type = BuildingType::TMPL;
    temple.name = "Temple of Nod";
    temple.sprite_base = "bld_temple_of_nod";
    temple.owner = HouseType::Nod;
    temple.hit_points = 1000;
    temple.cost = 3000;
    temple.build_time = 300;
    temple.sight_range = 5;
    temple.power_output = -100;
    temple.armor = ArmorType::Concrete;
    temple.width_cells = 3;
    temple.height_cells = 3;
    temple.has_bib = true;
    temple.tech_level = 9;
    temple.prerequisites = {BuildingType::NUKE, BuildingType::WEAP};

    // ---- Advanced Comm Center (EYE) ----
    auto& eye = buildings_[static_cast<int>(BuildingType::EYE)];
    eye.type = BuildingType::EYE;
    eye.name = "Advanced Comm Center";
    eye.sprite_base = "bld_comm_center";
    eye.owner = HouseType::GDI;
    eye.hit_points = 500;
    eye.cost = 2800;
    eye.build_time = 240;
    eye.sight_range = 8;
    eye.power_output = -80;
    eye.armor = ArmorType::Concrete;
    eye.width_cells = 2;
    eye.height_cells = 2;
    eye.has_bib = true;
    eye.tech_level = 9;
    eye.prerequisites = {BuildingType::NUKE, BuildingType::WEAP};

    // ---- Airstrip (AFLD) ----
    auto& airstrip = buildings_[static_cast<int>(BuildingType::AFLD)];
    airstrip.type = BuildingType::AFLD;
    airstrip.name = "Airstrip";
    airstrip.sprite_base = "bld_airstrip";
    airstrip.owner = HouseType::Nod;
    airstrip.hit_points = 500;
    airstrip.cost = 2000;
    airstrip.build_time = 150;
    airstrip.sight_range = 4;
    airstrip.power_output = -30;
    airstrip.armor = ArmorType::Wood;
    airstrip.width_cells = 3;
    airstrip.height_cells = 2;
    airstrip.has_bib = true;
    airstrip.produces_units = true;
    airstrip.tech_level = 3;
    airstrip.prerequisites = {BuildingType::NUKE};

    // ---- Helipad (HPAD) ----
    auto& hpad = buildings_[static_cast<int>(BuildingType::HPAD)];
    hpad.type = BuildingType::HPAD;
    hpad.name = "Helipad";
    hpad.sprite_base = "bld_helipad";
    hpad.owner = HouseType::GDI;
    hpad.hit_points = 400;
    hpad.cost = 1500;
    hpad.build_time = 120;
    hpad.sight_range = 4;
    hpad.power_output = -20;
    hpad.armor = ArmorType::Wood;
    hpad.width_cells = 2;
    hpad.height_cells = 2;
    hpad.has_bib = true;
    hpad.tech_level = 5;
    hpad.prerequisites = {BuildingType::WEAP};

    // ---- Repair Bay (REPAIR) ----
    auto& repair = buildings_[static_cast<int>(BuildingType::REPAIR)];
    repair.type = BuildingType::REPAIR;
    repair.name = "Repair Bay";
    repair.sprite_base = "bld_repair_bay";
    repair.hit_points = 400;
    repair.cost = 1200;
    repair.build_time = 100;
    repair.sight_range = 3;
    repair.power_output = -30;
    repair.armor = ArmorType::Concrete;
    repair.width_cells = 3;
    repair.height_cells = 2;
    repair.has_bib = true;
    repair.can_repair = true;
    repair.tech_level = 5;
    repair.prerequisites = {BuildingType::WEAP};

    // ---- Walls ----
    auto init_wall = [&](BuildingType type, const char* name,
                         const char* sprite, int cost, int hp, int tech) {
        auto& w = buildings_[static_cast<int>(type)];
        w.type = type;
        w.name = name;
        w.sprite_base = sprite;
        w.hit_points = hp;
        w.cost = cost;
        w.build_time = 10;
        w.sight_range = 1;
        w.armor = ArmorType::Concrete;
        w.width_cells = 1;
        w.height_cells = 1;
        w.is_wall = true;
        w.tech_level = tech;
    };

    init_wall(BuildingType::WALL, "Concrete Wall", "bld_concrete_wall",
              100, 200, 3);
    init_wall(BuildingType::SBAG, "Sandbag Wall", "bld_sandbag",
              25, 50, 1);
    init_wall(BuildingType::CYCL, "Chain Link Fence", "bld_chain_fence",
              50, 25, 1);
    init_wall(BuildingType::BRIK, "Concrete Barrier", "bld_concrete_barrier",
              100, 300, 5);
    init_wall(BuildingType::WOOD, "Wood Fence", "bld_wood_fence",
              25, 10, 1);
}

void GameDataRegistry::init_infantry() {
    infantry_.resize(static_cast<int>(InfantryType::Count));

    // ---- Minigunner (E1) ----
    auto& mini = infantry_[static_cast<int>(InfantryType::E1)];
    mini.type = InfantryType::E1;
    mini.name = "Minigunner";
    mini.sprite_base = "inf_minigunner";
    mini.hit_points = 50;
    mini.cost = 100;
    mini.build_time = 15;
    mini.sight_range = 3;
    mini.speed = 4;
    mini.armor = ArmorType::None;
    mini.tech_level = 1;
    mini.primary_weapon = {WeaponType::Machinegun, 10, 3, 20, "proj_bullet", "snd_machinegun"};

    // ---- Grenadier (E2) ----
    auto& gren = infantry_[static_cast<int>(InfantryType::E2)];
    gren.type = InfantryType::E2;
    gren.name = "Grenadier";
    gren.sprite_base = "inf_grenadier";
    gren.hit_points = 50;
    gren.cost = 160;
    gren.build_time = 20;
    gren.sight_range = 3;
    gren.speed = 3;
    gren.armor = ArmorType::None;
    gren.tech_level = 2;
    gren.primary_weapon = {WeaponType::Grenade, 30, 4, 40, "proj_grenade", "snd_grenade"};

    // ---- Rocket Soldier (E3) ----
    auto& rocket = infantry_[static_cast<int>(InfantryType::E3)];
    rocket.type = InfantryType::E3;
    rocket.name = "Rocket Soldier";
    rocket.sprite_base = "inf_rocket_soldier";
    rocket.hit_points = 45;
    rocket.cost = 300;
    rocket.build_time = 30;
    rocket.sight_range = 4;
    rocket.speed = 3;
    rocket.armor = ArmorType::None;
    rocket.tech_level = 3;
    rocket.primary_weapon = {WeaponType::Rocket, 30, 5, 50, "proj_missile", "snd_rocket"};

    // ---- Flamethrower (E4) ----
    auto& flamer = infantry_[static_cast<int>(InfantryType::E4)];
    flamer.type = InfantryType::E4;
    flamer.name = "Flamethrower";
    flamer.sprite_base = "inf_flamethrower";
    flamer.owner = HouseType::Nod;
    flamer.hit_points = 60;
    flamer.cost = 200;
    flamer.build_time = 25;
    flamer.sight_range = 3;
    flamer.speed = 3;
    flamer.armor = ArmorType::None;
    flamer.tech_level = 3;
    flamer.primary_weapon = {WeaponType::Flamethrower, 35, 2, 25, "proj_flame", "snd_flame"};

    // ---- Chem Warrior (E5) ----
    auto& chem = infantry_[static_cast<int>(InfantryType::E5)];
    chem.type = InfantryType::E5;
    chem.name = "Chem Warrior";
    chem.sprite_base = "inf_chem_warrior";
    chem.owner = HouseType::Nod;
    chem.hit_points = 70;
    chem.cost = 300;
    chem.build_time = 30;
    chem.sight_range = 3;
    chem.speed = 3;
    chem.armor = ArmorType::None;
    chem.tech_level = 8;
    chem.primary_weapon = {WeaponType::ChemSpray, 40, 3, 30, "proj_chem", "snd_chem"};

    // ---- Engineer (E6) ----
    auto& eng = infantry_[static_cast<int>(InfantryType::E6)];
    eng.type = InfantryType::E6;
    eng.name = "Engineer";
    eng.sprite_base = "inf_engineer";
    eng.hit_points = 25;
    eng.cost = 500;
    eng.build_time = 30;
    eng.sight_range = 2;
    eng.speed = 3;
    eng.armor = ArmorType::None;
    eng.is_engineer = true;
    eng.can_capture = true;
    eng.tech_level = 4;

    // ---- Commando (E7) ----
    auto& commando = infantry_[static_cast<int>(InfantryType::E7)];
    commando.type = InfantryType::E7;
    commando.name = "Commando";
    commando.sprite_base = "inf_commando";
    commando.hit_points = 80;
    commando.cost = 1000;
    commando.build_time = 60;
    commando.sight_range = 5;
    commando.speed = 5;
    commando.armor = ArmorType::None;
    commando.is_commando = true;
    commando.can_c4 = true;
    commando.tech_level = 10;
    commando.primary_weapon = {WeaponType::SniperRifle, 100, 5, 50, "proj_bullet", "snd_sniper"};
}

} // namespace CnC

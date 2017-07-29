#ifndef SHIP_DEFINITIONS_HPP_INCLUDED
#define SHIP_DEFINITIONS_HPP_INCLUDED

#define default_room_hp 10.f

static float efficiency_cost_exponent = 1.5f;

inline float get_cost_mod(float cost)
{
    if(cost < 1)
        return cost;

    return pow(cost, efficiency_cost_exponent);
}

inline component make_default_crew()
{
    component_attribute command;
    command.produced_per_s = 4.f;

    component_attribute oxygen;
    oxygen.drained_per_s = 0.5f;
    oxygen.max_amount = 10.f;
    //oxygen.cur_amount = oxygen.max_amount;

    //component_attribute repair;
    //repair.produced_per_s = 0.2f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;
    hp.produced_per_s = 0.1f;

    component_attribute armour;
    armour.produced_per_s = 0.05f;

    component crew;
    crew.add(ship_component_element::COMMAND, command);
    crew.add(ship_component_element::OXYGEN, oxygen);
    //crew.add(ship_component_element::REPAIR, repair);
    crew.add(ship_component_element::HP, hp);
    crew.add(ship_component_element::ARMOUR, armour);

    crew.name = "Crew";
    crew.primary_attribute = ship_component_elements::COMMAND;
    crew.repair_this_when_recrewing = true;

    crew.set_tag(component_tag::DAMAGED_WITHOUT_O2, 0.5f);
    crew.set_tag(component_tag::OXYGEN_STARVATION, 0.0f);

    return crew;
}

inline component make_default_life_support()
{
    component_attribute oxygen;
    oxygen.produced_per_s = 0.7f;

    component_attribute power;
    power.drained_per_s = 5.f;
    power.max_amount = 10.f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component life_support;
    life_support.add(ship_component_element::OXYGEN, oxygen);
    life_support.add(ship_component_element::ENERGY, power);
    life_support.add(ship_component_element::HP, hp);

    life_support.name = "Life Support";
    life_support.primary_attribute = ship_component_elements::OXYGEN;
    life_support.repair_this_when_recrewing = true;

    return life_support;
}

inline component make_default_ammo_store()
{
    component_attribute ammo;
    ammo.max_amount = 10.f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component ammo_store;
    ammo_store.add(ship_component_element::AMMO, ammo);
    ammo_store.add(ship_component_element::HP, hp);

    ammo_store.name = "Ammo Store";
    ammo_store.skip_in_derelict_calculations = true;
    ammo_store.primary_attribute = ship_component_elements::AMMO;

    return ammo_store;
}

inline component make_default_shields(float effectiveness = 1.f)
{
    component_attribute shield;
    shield.produced_per_s = 1.f * effectiveness;
    shield.max_amount = 30.f * effectiveness;

    component_attribute power;
    power.drained_per_s = 20.f * effectiveness;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component shields;
    shields.add(ship_component_element::SHIELD_POWER, shield);
    shields.add(ship_component_element::ENERGY, power);
    shields.add(ship_component_element::HP, hp);

    shields.name = "Shield Generator";
    shields.primary_attribute = ship_component_elements::SHIELD_POWER;

    shields.cost_mult = get_cost_mod(effectiveness);

    return shields;
}

inline component make_default_power_core(float effectiveness = 1.f)
{
    component_attribute power;
    power.produced_per_s = 80.f * effectiveness;
    power.max_amount = 80.f * effectiveness;

    component_attribute fuel;
    fuel.max_amount = 1.f;
    fuel.cur_amount = 1.f;
    fuel.drained_per_s = effectiveness * fuel.max_amount / 1000;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;
    //hp.cur_amount = 0;

    component core;
    core.add(ship_component_element::ENERGY, power);
    core.add(ship_component_element::HP, hp);
    core.add(ship_component_element::FUEL, fuel);

    core.name = "Power Core";
    core.primary_attribute = ship_component_elements::ENERGY;
    core.repair_this_when_recrewing = true;
    core.cost_mult = get_cost_mod(effectiveness);

    return core;
}

inline component make_default_engines()
{
    float max_bad_engine_lifetime_s = 1000;

    component_attribute fuel;
    fuel.max_amount = 10.f;
    fuel.cur_amount = 4.f;
    fuel.drained_per_s = fuel.max_amount/max_bad_engine_lifetime_s;

    component_attribute power;
    power.drained_per_s = 10.f;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute engine;
    engine.produced_per_s = 1.f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component thruster;
    thruster.add(ship_component_element::FUEL, fuel);
    thruster.add(ship_component_element::ENERGY, power);
    thruster.add(ship_component_element::COMMAND, command);
    thruster.add(ship_component_element::ENGINE_POWER, engine);
    thruster.add(ship_component_element::HP, hp);

    thruster.name = "Thruster";
    thruster.primary_attribute = ship_component_elements::ENGINE_POWER;

    return thruster;
}

inline component make_default_heatsink(float efficiency = 1.f)
{
    component_attribute cooling;
    cooling.max_amount = 80 * efficiency;
    cooling.produced_per_s = 1.1f * efficiency;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component heatsink;
    heatsink.add(ship_component_element::COOLING_POTENTIAL, cooling);
    heatsink.add(ship_component_element::HP, hp);

    heatsink.name = "Heatsink";
    heatsink.skip_in_derelict_calculations = true;
    heatsink.primary_attribute = ship_component_elements::COOLING_POTENTIAL;
    heatsink.cost_mult = get_cost_mod(efficiency);

    return heatsink;
}

inline component make_default_warp_drive(float charge_rate = 1.f)
{
    component_attribute cooling;
    cooling.drained_per_s = 0.1f;
    cooling.drained_per_use = 10.f;

    component_attribute power;
    power.drained_per_use = 80;
    power.drained_per_s = 15.f * charge_rate;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute hp;
    hp.max_amount = default_room_hp/4.f;
    hp.cur_amount = hp.max_amount;

    component_attribute warp_power;
    warp_power.produced_per_s = 0.5f * charge_rate;
    warp_power.drained_per_use = 10.f;
    warp_power.max_amount = 10.f;
    warp_power.cur_amount = warp_power.max_amount * 0.25f;

    component_attribute fuel;
    fuel.drained_per_use = 1.f;

    ///what I'd really like to do here is say "if we don't have enough shields, drain hp"
    ///but this is too complex to express at the moment
    //component_attribute shield;
    //shield.drained_per_use = 5;

    component warp_drive;
    warp_drive.add(ship_component_element::COOLING_POTENTIAL, cooling);
    warp_drive.add(ship_component_element::ENERGY, power);
    warp_drive.add(ship_component_element::COMMAND, command);
    warp_drive.add(ship_component_element::HP, hp);
    warp_drive.add(ship_component_element::WARP_POWER, warp_power);
    warp_drive.add(ship_component_element::FUEL, fuel);

    warp_drive.name = "Warp Drive";
    warp_drive.primary_attribute = ship_component_elements::WARP_POWER;
    warp_drive.cost_mult = get_cost_mod(charge_rate);
    warp_drive.set_tag(component_tag::WARP_DISTANCE, 40.f);

    return warp_drive;
}

inline component make_default_stealth(float effectiveness = 1.f)
{
    component_attribute cooling;
    cooling.drained_per_s = 0.2f * effectiveness;

    component_attribute power;
    power.drained_per_s = 10.f * effectiveness;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component_attribute stealth;
    stealth.produced_per_s = 30.f * effectiveness;

    component stealth_drive;
    stealth_drive.add(ship_component_element::COOLING_POTENTIAL, cooling);
    stealth_drive.add(ship_component_element::ENERGY, power);
    stealth_drive.add(ship_component_element::COMMAND, command);
    stealth_drive.add(ship_component_element::HP, hp);
    stealth_drive.add(ship_component_element::STEALTH, stealth);

    stealth_drive.scanning_difficulty = clamp(stealth_drive.scanning_difficulty*2, 0.f, 1.f);

    stealth_drive.name = "Stealth Systems";
    stealth_drive.primary_attribute = ship_component_element::STEALTH;
    stealth_drive.cost_mult = get_cost_mod(effectiveness);

    return stealth_drive;
}

inline component make_default_coloniser()
{
    component_attribute cooling;
    cooling.drained_per_s = 1.1f;

    component_attribute power;
    power.drained_per_use = 80;
    power.drained_per_s = 70.f;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute hp;
    hp.max_amount = default_room_hp/4.f;
    hp.cur_amount = hp.max_amount;

    component_attribute colony;
    colony.produced_per_use = 1.f;

    ///what I'd really like to do here is say "if we don't have enough shields, drain hp"
    ///but this is too complex to express at the moment
    //component_attribute shield;
    //shield.drained_per_use = 5;

    component coloniser;
    coloniser.add(ship_component_element::COOLING_POTENTIAL, cooling);
    coloniser.add(ship_component_element::ENERGY, power);
    coloniser.add(ship_component_element::COMMAND, command);
    coloniser.add(ship_component_element::HP, hp);
    coloniser.add(ship_component_element::COLONISER, colony);

    coloniser.name = "Coloniser";
    coloniser.primary_attribute = ship_component_elements::COLONISER;

    return coloniser;
}

inline component make_default_scanner(float effectiveness = 1.f)
{
    component_attribute cooling;
    cooling.drained_per_s = 0.1f * effectiveness;

    component_attribute power;
    power.drained_per_s = 5 * effectiveness;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component_attribute scanner_power;
    scanner_power.produced_per_s = 20 * effectiveness;

    component scanner;

    scanner.add(ship_component_element::COOLING_POTENTIAL, cooling);
    scanner.add(ship_component_element::ENERGY, power);
    scanner.add(ship_component_element::COMMAND, command);
    scanner.add(ship_component_element::HP, hp);
    scanner.add(ship_component_element::SCANNING_POWER, scanner_power);

    scanner.name = "Scanner";
    scanner.primary_attribute = ship_component_element::SCANNING_POWER;
    scanner.cost_mult = get_cost_mod(effectiveness);

    return scanner;
}

inline component make_default_railgun()
{
    ///...the railgun literally produces railgun
    component_attribute railgun;
    railgun.produced_per_use = 1;
    railgun.time_between_uses_s = 24;

    ///ok so... positive heat is good,negative is bad
    ///rename cooling?
    component_attribute cooling;
    cooling.drained_per_use = 20.f;
    cooling.produced_per_s = 0.1f;
    cooling.max_amount = 10.f;

    component_attribute power;
    power.drained_per_s = 5;
    power.drained_per_use = 50.f;

    component_attribute command;
    command.drained_per_s = 1.f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component gun;
    gun.add(ship_component_element::RAILGUN, railgun);
    gun.add(ship_component_element::COOLING_POTENTIAL, cooling);
    gun.add(ship_component_element::ENERGY, power);
    gun.add(ship_component_element::COMMAND, command);
    gun.add(ship_component_element::HP, hp);

    gun.set_tag(component_tag::DAMAGE, 20.f);
    gun.set_tag(component_tag::SPEED, 80.f);
    gun.set_tag(component_tag::SCALE, 0.5f);

    gun.name = "Railgun";
    gun.primary_attribute = ship_component_elements::RAILGUN;

    return gun;
}

inline component make_default_torpedo()
{
    component_attribute torpedo;
    torpedo.produced_per_use = 1;
    torpedo.time_between_uses_s = 16;

    component_attribute cooling;
    cooling.drained_per_use = 2.f;
    cooling.produced_per_s = 0.01f;
    cooling.max_amount = 1;

    component_attribute power;
    power.drained_per_s = 1;
    power.drained_per_use = 5;

    component_attribute command;
    command.drained_per_s = 1;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component torp;
    torp.add(ship_component_element::TORPEDO, torpedo);
    torp.add(ship_component_element::COOLING_POTENTIAL, cooling);
    torp.add(ship_component_element::ENERGY, power);
    torp.add(ship_component_element::COMMAND, command);
    torp.add(ship_component_element::HP, hp);

    torp.set_tag(component_tag::DAMAGE, 25);
    torp.set_tag(component_tag::SPEED, 40);
    torp.set_tag(component_tag::SCALE, 0.5f);

    torp.name = "Torpedo";
    torp.primary_attribute = ship_component_elements::TORPEDO;

    return torp;
}

inline ship make_default()
{
    /*ship test_ship;

    component_attribute powerplanet_core;
    powerplanet_core.produced_per_s = 1;

    component_attribute energy_sink;
    energy_sink.drained_per_s = 0.1f;
    energy_sink.max_amount = 50.f;

    component_attribute energy_sink2;
    energy_sink2.drained_per_s = 0.2f;
    energy_sink2.max_amount = 50.f;

    component_attribute powerplant_core2;
    powerplant_core2.produced_per_s = 2;

    component powerplant;
    powerplant.add(ship_component_element::ENERGY, powerplanet_core);

    component powerplant2;
    powerplant2.add(ship_component_element::ENERGY, powerplant_core2);

    component battery;
    battery.add(ship_component_element::ENERGY, energy_sink);

    component battery2;
    battery2.add(ship_component_element::ENERGY, energy_sink2);

    test_ship.add(powerplant);
    test_ship.add(battery);
    test_ship.add(battery2);
    test_ship.add(powerplant2);*/

    ship test_ship;
    test_ship.add(make_default_crew());
    test_ship.add(make_default_life_support());
    test_ship.add(make_default_ammo_store());
    test_ship.add(make_default_shields());
    test_ship.add(make_default_power_core());
    test_ship.add(make_default_engines());
    test_ship.add(make_default_warp_drive());
    test_ship.add(make_default_scanner());
    test_ship.add(make_default_heatsink());
    test_ship.add(make_default_railgun());
    test_ship.add(make_default_torpedo());
    test_ship.add(make_default_stealth());

    test_ship.name = "Military Default";

    return test_ship;
}

///need a modifier for making things worse or better
inline ship make_civilian()
{
    ship test_ship;
    test_ship.add(make_default_crew());
    test_ship.add(make_default_life_support());
    test_ship.add(make_default_shields());
    test_ship.add(make_default_power_core());
    test_ship.add(make_default_engines());
    test_ship.add(make_default_warp_drive());
    test_ship.add(make_default_scanner());
    test_ship.add(make_default_heatsink());

    test_ship.name = "Civilian Default";

    return test_ship;
}

inline ship make_scout()
{
    ship test_ship;
    test_ship.add(make_default_crew());
    test_ship.add(make_default_life_support());
    test_ship.add(make_default_ammo_store());
    test_ship.add(make_default_shields(0.25f));
    test_ship.add(make_default_power_core(0.8f));
    test_ship.add(make_default_engines());
    test_ship.add(make_default_warp_drive(1.25f));
    test_ship.add(make_default_scanner(1.5f));
    test_ship.add(make_default_heatsink(0.75f));
    test_ship.add(make_default_torpedo());
    test_ship.add(make_default_stealth(1.5f));

    test_ship.name = "Scout Default";

    return test_ship;
}

inline ship make_colony_ship()
{
    ship test_ship;
    test_ship.add(make_default_crew());
    test_ship.add(make_default_life_support());
    test_ship.add(make_default_power_core(1.4f));
    test_ship.add(make_default_engines());
    test_ship.add(make_default_warp_drive(0.75f));
    test_ship.add(make_default_scanner(0.5));
    test_ship.add(make_default_heatsink(1.8f));
    test_ship.add(make_default_coloniser());

    test_ship.name = "Colony Default";

    return test_ship;
}

#endif // SHIP_DEFINITIONS_HPP_INCLUDED

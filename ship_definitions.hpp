#ifndef SHIP_DEFINITIONS_HPP_INCLUDED
#define SHIP_DEFINITIONS_HPP_INCLUDED

#define default_room_hp 10.f

component make_default_crew()
{
    component_attribute command;
    command.produced_per_s = 4.f;

    component_attribute oxygen;
    oxygen.drained_per_s = 0.5f;
    oxygen.max_amount = 5.f;

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

    return crew;
}

component make_default_life_support()
{
    component_attribute oxygen;
    oxygen.produced_per_s = 2.f;

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

    return life_support;
}

component make_default_ammo_store()
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

    return ammo_store;
}

component make_default_shields()
{
    component_attribute shield;
    shield.produced_per_s = 1.f;
    shield.max_amount = 50.f;

    component_attribute power;
    power.drained_per_s = 20.f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component shields;
    shields.add(ship_component_element::SHIELD_POWER, shield);
    shields.add(ship_component_element::ENERGY, power);
    shields.add(ship_component_element::HP, hp);

    shields.name = "Shield Generator";

    return shields;
}

component make_default_power_core()
{
    component_attribute power;
    power.produced_per_s = 80.f;
    power.max_amount = 80.f;

    component_attribute fuel;
    fuel.max_amount = 1.f;
    fuel.cur_amount = 1.f;
    fuel.drained_per_s = fuel.max_amount / 1000;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component core;
    core.add(ship_component_element::ENERGY, power);
    core.add(ship_component_element::HP, hp);
    core.add(ship_component_element::FUEL, fuel);

    core.name = "Power Core";

    return core;
}

component make_default_engines()
{
    float max_bad_engine_lifetime_s = 50;

    component_attribute fuel;
    fuel.max_amount = 10.f;
    fuel.cur_amount = 2.f;
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

    return thruster;
}

component make_default_heatsink()
{
    component_attribute cooling;
    cooling.max_amount = 80;
    cooling.produced_per_s = 1.1f;

    component_attribute hp;
    hp.max_amount = default_room_hp;
    hp.cur_amount = hp.max_amount;

    component heatsink;
    heatsink.add(ship_component_element::COOLING_POTENTIAL, cooling);
    heatsink.add(ship_component_element::HP, hp);

    heatsink.name = "Heatsink";

    return heatsink;
}

component make_default_warp_drive()
{
    component_attribute cooling;
    cooling.drained_per_s = 0.1f;
    cooling.drained_per_use = 10.f;

    component_attribute power;
    power.drained_per_use = 80;

    component_attribute command;
    command.drained_per_s = 0.5f;

    component_attribute hp;
    hp.max_amount = default_room_hp/4.f;
    hp.cur_amount = hp.max_amount;

    component_attribute warp_power;
    warp_power.produced_per_s = 10.25f;
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

    return warp_drive;
}

component make_default_railgun()
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

    gun.set_tag(component_tag::DAMAGE, 30.f);
    gun.set_tag(component_tag::SPEED, 80.f);
    gun.set_tag(component_tag::SCALE, 0.5f);

    gun.name = "Railgun";

    return gun;
}

component make_default_torpedo()
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

    torp.set_tag(component_tag::DAMAGE, 35);
    torp.set_tag(component_tag::SPEED, 40);
    torp.set_tag(component_tag::SCALE, 0.5f);

    torp.name = "Torpedo";

    return torp;
}

ship make_default()
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
    test_ship.add(make_default_heatsink());
    test_ship.add(make_default_railgun());
    test_ship.add(make_default_torpedo());

    return test_ship;
}

#endif // SHIP_DEFINITIONS_HPP_INCLUDED

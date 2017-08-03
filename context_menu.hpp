#ifndef CONTEXT_MENU_HPP_INCLUDED
#define CONTEXT_MENU_HPP_INCLUDED

struct contextable
{
    bool currently_going = false;
};

struct context_menu
{
    static contextable* current;

    static void set_item(contextable* item) {if(current) {current->currently_going = false;} current = item;}

    static void start();

    static void stop();
};

#endif // CONTEXT_MENU_HPP_INCLUDED

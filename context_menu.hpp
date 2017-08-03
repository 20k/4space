#ifndef CONTEXT_MENU_HPP_INCLUDED
#define CONTEXT_MENU_HPP_INCLUDED

///maybe have contextable have a render function and overload?
struct contextable
{
    bool context_going = false;

    virtual void process_context_ui();

    virtual ~contextable() {}
};

struct context_menu
{
    static contextable* current;

    static void set_item(contextable* item)
    {
        if(current)
        {
            current->context_going = false;
        }

        current = item;

        if(current == nullptr)
            return;

        current->context_going = true;
    }

    static void start();

    static void tick()
    {
        if(current)
        {
            current->process_context_ui();
        }
    }

    static void stop();
};

#endif // CONTEXT_MENU_HPP_INCLUDED

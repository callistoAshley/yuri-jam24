Start curly braces on new lines.

lower_snake_case variable and function names.

    int thing = 1;
    void some_function(char some_param, int another_param)
    {
        int another_thing;
    }

UPPER_SNAKE_CASE macro names.

    #define SOME_MACRO 5
    #define DO_SOMETHING

Names of non-static functions should begin with some indicator as to what file they belong to. For example, in a theoretical "foo.c":
    
    void foo_do_something()
    {
        
    }

    void foo_do_something_else()
    {
        
    }

    static void some_static_function()
    {

    }

Local variables should be declared at the start of whatever scope they belong to.

    int some_variable;
    char some_other_variable;

    void some_function()
    {
        int another_variable;

        if (true)
        {
            int p;
            char c;

            p = &another_variable;
            c = 'a';
        }
    }

Indent with 4 spaces.

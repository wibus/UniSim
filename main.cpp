#include <iostream>

#include "universe.h"


int main(int argc, char ** argv)
{
    return unisim::Universe::getInstance().launch(argc, argv);
}

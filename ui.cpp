#include "ui.h"


namespace unisim
{

Ui::Ui(bool showUi) :
    _showUi(showUi)
{

}

Ui::~Ui()
{

}


void Ui::show()
{
    _showUi = true;
}

void Ui::hide()
{
    _showUi = false;
}

}

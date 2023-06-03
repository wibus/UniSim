#ifndef SOLAR_SYSTEM_UI_H
#define SOLAR_SYSTEM_UI_H

#include "../ui.h"


namespace unisim
{

class SolarSystemUi : public Ui
{
public:
    SolarSystemUi();
    virtual ~SolarSystemUi();

    void render(Scene& scene) override;

private:
};

}

#endif // SOLAR_SYSTEM_UI_H

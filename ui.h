#ifndef UI_H
#define UI_H



namespace unisim
{

class Scene;


class Ui
{
public:
    Ui(bool showUi = false);
    virtual ~Ui();

    void show();
    void hide();
    bool isShown() const { return _showUi; }

    virtual void render(Scene& scene) = 0;

protected:
    bool _showUi;
};

}

#endif // UI_H

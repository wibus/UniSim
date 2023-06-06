#ifndef UI_H
#define UI_H



namespace unisim
{

class Scene;
class CameraMan;


class Ui
{
public:
    Ui(bool showUi = false);
    virtual ~Ui();

    void show();
    void hide();
    bool isShown() const { return _showUi; }

    virtual void render(Scene& scene, CameraMan& cameraMan);

protected:
    bool _showUi;
};

}

#endif // UI_H

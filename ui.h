#ifndef UI_H
#define UI_H



namespace unisim
{

class Scene;
class Camera;
class ResourceManager;


class Ui
{
public:
    Ui(bool showUi = false);
    virtual ~Ui();

    void show();
    void hide();
    bool isShown() const { return _showUi; }

    virtual void render(const ResourceManager& resources, Scene& scene, Camera &camera);

protected:
    bool _showUi;
};

}

#endif // UI_H

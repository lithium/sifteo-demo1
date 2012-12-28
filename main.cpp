

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;


// METADATA

static Metadata M = Metadata()
    .title("Li-demo1")
    .package("lithium.demo1", "0.1")
    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapAssets)
    ;

static AssetLoader loader;
static AssetConfiguration<1> config;

static CubeSet activeCubes;
static CubeSet newCubes;

class LiDemo1 {
public:
    void init(CubeID cid)
    {
        activeCubes.mark(cid);
        vbuf.attach(cid);
        vbuf.initMode(SOLID_MODE);
    }

    void update(TimeDelta step)
    {
        vbuf.initMode(SOLID_MODE);
    }

    void connect(CubeID cid)
    {
        newCubes.mark(cid);
        vbuf.attach(cid);
        vbuf.initMode(BG0_ROM);
        vbuf.bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
        vbuf.bg0rom.text(vec(1,7), "Arise cube!", BG0ROMDrawable::RED);
    }

    void load_progress(CubeID cid)
    {
        vbuf.bg0rom.hBargraph(vec(0,4), loader.cubeProgress(cid, 128), BG0ROMDrawable::RED, 8);
    }



public:
private:
    VideoBuffer vbuf;

} demo_cubes[CUBE_ALLOCATION];



static void cube_connect(void *ctx, unsigned cid) {
    demo_cubes[cid].connect(cid);
}
static void cube_disconnect(void *ctx, unsigned cid) {
    newCubes.clear(cid);
    activeCubes.clear(cid);
}


void main()
{
    config.append(MainSlot, BootstrapAssets);
    loader.init();

    Events::cubeConnect.set(cube_connect);
    Events::cubeDisconnect.set(cube_disconnect);
    // Events::cubeRefresh.set(cube_refresh);

    AudioTracker::play(Music);
    for (CubeID cid : CubeSet::connected()) {
        demo_cubes[cid].init(cid);
    }

    TimeStep ts;
    for (;;) {
        newCubes.clear();

        System::paint();

        if (!newCubes.empty()) {
            AudioTracker::pause();
            loader.start(config);
            while (!loader.isComplete()) {
                for (CubeID cid : newCubes) {
                    demo_cubes[cid].load_progress(cid);
                }
                System::paint();
            }
            loader.finish();
            for (CubeID cid : newCubes) {
                newCubes.clear(cid);
                demo_cubes[cid].init(cid);
            }
            AudioTracker::resume();
        }


        for (CubeID cid : activeCubes) {
            demo_cubes[cid].update(ts.delta());
        }
        ts.next();
    }
}
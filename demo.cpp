


#define NUM_CUBES CUBE_ALLOCATION
#define NUM_STARS CUBE_ALLOCATION

static Random rand;

class DemoCube {
public:
    CubeID id;
    VideoBuffer vbuf;
    unsigned n_sprites;
    bool dirty;

    DemoCube() : dirty(true) 
    {}

    void init(CubeID id) 
    {
        this->id = id;
        vbuf.attach(this->id);

        vbuf.initMode(BG0_SPR_BG1);
        vbuf.bg0.image(vec(0,0), Background);
    }
    void paint(unsigned frame) {
    }

    void loading(unsigned progress)
    {
        vbuf.initMode(BG0_ROM);
        vbuf.bg0rom.hBargraph(vec(0,16), progress, BG0ROMDrawable::BLUE);
    }
};


class DemoStar {
public:
    unsigned id;
    Float2 position,velocity;
    DemoCube *cube;

    void init(DemoCube *cube)
    {
        const float max_spd = 80.0f; 

        this->cube = cube;
        id = this->cube->n_sprites++;
        position.set(0,0);
        velocity.setPolar(rand.uniform(0,2*M_PI), rand.uniform(max_spd*0.5f,max_spd));
    }

    void paint(unsigned frame)
    {
        const Float2 center = {64 - 3.5f, 64 - 3.5f};

        cube->vbuf.sprites[id].setImage(Star, frame % Star.numFrames());
        cube->vbuf.sprites[id].move(position + center);
    }

    void tick(TimeDelta step)
    {

        if (position.x > 64 || position.x < -64) {
            velocity.x *= -1;
        } 
        if (position.y > 64 || position.y < -64) {
            velocity.y *= -1;
        }
        position += float(step) * velocity;

    }

};


extern AssetSlot MainSlot;


class Demo {
public:

    Demo() : running(true) {
        config.append(MainSlot, DemoAssets);
        loader.init();
    }

    void init() 
    {

        cubes_new = CubeSet::connected();
        CubeSet connected = CubeSet::connected();
        unsigned n_cubes = connected.count();

        for (unsigned i = 0; i < NUM_STARS; i++) {
            stars[i].init(&cubes[i % n_cubes]);
        }

        Events::cubeConnect.set(&Demo::onCubeConnect, this);
        Events::cubeDisconnect.set(&Demo::onCubeDisconnect, this);
        Events::neighborAdd.set(&Demo::onNeighborAdd, this);
        Events::neighborRemove.set(&Demo::onNeighborRemove, this);
        Events::gameMenu.set(&Demo::onRestart, this, "Restart");

    }
    void cleanup()
    {
        AudioTracker::stop();

        Events::cubeConnect.unset();
        Events::cubeDisconnect.unset();
        Events::neighborAdd.unset();
        Events::neighborRemove.unset();
        Events::gameMenu.unset();

    }
    void paint()
    {
        for (CubeID cid : cubes_active) {
            cubes[cid].paint(frame);
        }
        for (unsigned i = 0; i < NUM_STARS; i++) {
            stars[i].paint(frame);
        }
    }
    void tick(TimeDelta step)
    {
        for (unsigned i = 0; i < NUM_STARS; i++) {
            stars[i].tick(step);
        }
    }


    void run()
    {
        AudioTracker::play(Music);

        TimeStep ts;
        frame = 0;
        running = true;
        while (running) {

            if (!cubes_new.empty()) {
                AudioTracker::pause();
                loader.start(config);
                while (!loader.isComplete()) {
                    for (CubeID cid : cubes_new) {
                        cubes[cid].loading(loader.cubeProgress(cid, 128));
                    }
                    System::paint();
                }
                loader.finish();
                for (CubeID cid : cubes_new) {
                    cubes[cid].init(cid);
                }
                AudioTracker::resume();
                cubes_new.clear(); 
            }

            ts.next();

            tick(ts.delta());
            paint();

            System::paint();

            frame++;
        }

    }


private:
    AssetLoader loader;
    AssetConfiguration<1> config;

    DemoCube cubes[NUM_CUBES];
    DemoStar stars[NUM_STARS];

    bool running;
    unsigned frame;

    CubeSet cubes_active;
    CubeSet cubes_new;

    void onCubeConnect(unsigned cid)
    {
        cubes_new.mark(cid);
        cubes[cid].loading(0);
    }
    void onCubeDisconnect(unsigned cid) {
        cubes_active.clear(cid);
        cubes_new.clear(cid);
    }
    void onNeighborAdd(unsigned c0, unsigned s0, unsigned c1, unsigned s1) {}
    void onNeighborRemove(unsigned c0, unsigned s0, unsigned c1, unsigned s1) {}
    void onRestart() {
        running = false;
    }
};
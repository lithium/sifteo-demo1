


#define NUM_CUBES CUBE_ALLOCATION
#define NUM_STARS CUBE_ALLOCATION

static Random rand;


class DemoCube;

class Portal {
public:
    Portal(int side) : state(S_CLOSED), side(side), frame(0)
    {}

    void open(DemoCube *dest_cube, unsigned dest_side)
    {
        if (state != S_OPEN)
            state = S_OPENING;
        this->dest_cube = dest_cube;
        this->dest_side = dest_side;
    }
    void close()
    {
        if (state != S_CLOSED)
            state = S_CLOSING;
        this->dest_cube = NULL;
    }

    void tick()
    {
        const unsigned max_frame = Portal_n.numFrames() - 1;

        switch (state) {
        case S_CLOSING:
            if (!frame || !--frame)
                state = S_CLOSED;
            break;

        case S_OPENING:
            if (frame >= max_frame || ++frame >= max_frame)
                state = S_OPEN;
            break;

        case S_CLOSED:
            frame = 0;
            break;

        case S_OPEN:
            if (frame == max_frame)
                frame = max_frame - 1;
            else
                frame = max_frame;
            break;
        }

    }
    void paint(VideoBuffer &vbuf)
    {
        static const struct {
            const AssetImage *asset;
            Byte2 pos;
        } info[NUM_SIDES] = {
            { &Portal_n, { 1,  0  } },
            { &Portal_w, { 0,  1  } },
            { &Portal_s, { 1,  15 } },
            { &Portal_e, { 15, 1  } },
        };


        vbuf.bg0.image(info[side].pos, *info[side].asset, frame);
    }

    enum {
        S_CLOSED,
        S_CLOSING,
        S_OPEN,
        S_OPENING,
    } state;
    uint8_t side;
    uint8_t frame;

    DemoCube *dest_cube;
    uint8_t dest_side;
};

class DemoCube {
public:
    CubeID id;
    VideoBuffer vbuf;
    BitArray<8> sprite_set;
    DemoCube *cubes;

    Portal portal_n,portal_s,portal_e,portal_w;
    Portal &getPortal(unsigned i) {
        switch (i) {
        default:
        case TOP:     return portal_n;
        case LEFT:    return portal_w;
        case BOTTOM:  return portal_s;
        case RIGHT:   return portal_e;
        };
    }


    DemoCube() : sprite_set(0,7), portal_n(TOP), portal_s(BOTTOM), portal_e(RIGHT), portal_w(LEFT)
    {
    }

    void init(CubeID id, DemoCube *cubes) 
    {
        this->id = id;
        this->cubes = cubes;
        vbuf.attach(this->id);

        vbuf.initMode(BG0_SPR_BG1);
        sprite_set.mark();
    }
    void paint(unsigned frame) {

        vbuf.bg0.image(vec(0,0), Background);
        for (unsigned i = 0; i < NUM_SIDES; i++) {
            Portal &p = getPortal(i);
            p.paint(vbuf);
        }
    }
    void tick(TimeDelta step) {
        for (unsigned i = 0; i < NUM_SIDES; i++) {
            Portal &p = getPortal(i);
            p.tick();
        }
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


    void switch_cubes(DemoCube *dest, unsigned side)
    {
        this->cube->vbuf.sprites.erase();

        this->cube->sprite_set.mark(this->id);
        dest->sprite_set.findFirst(this->id);
        dest->sprite_set.clear(this->id);
        this->cube = dest;


        switch (side) {
        case LEFT:
            position.x = -60;
            break;
        case RIGHT:
            position.x = 60;
            break;
        case TOP:
            position.y = -60;
            break;
        case BOTTOM:
            position.y = 60;
            break;
        }
    }

    void init(DemoCube *cube)
    {
        const float max_spd = 80.0f; 

        this->cube = cube;
        this->cube->sprite_set.findFirst(id);
        this->cube->sprite_set.clear(id);
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
        Neighborhood hood(cube->id);

        if (position.x > 60) {
            Portal p = this->cube->getPortal(RIGHT);
            if (p.dest_cube) {
                if (!p.dest_cube->sprite_set.empty()) {
                    switch_cubes(p.dest_cube, p.dest_side);
                }
                else {
                    velocity.x *= -1;
                }
            }
            else {
                velocity.x *= -1;
            }

        }
        if (position.x < -60) {
            Portal p = this->cube->getPortal(LEFT);
            if (p.dest_cube) {
                if (!p.dest_cube->sprite_set.empty()) {
                    switch_cubes(p.dest_cube, p.dest_side);
                }
                else {
                    velocity.x *= -1;
                }
            }
            else {
                velocity.x *= -1;
            }

        }
        if (position.y > 60) {
            Portal p = this->cube->getPortal(BOTTOM);
            if (p.dest_cube) {
                if (!p.dest_cube->sprite_set.empty()) {
                    switch_cubes(p.dest_cube, p.dest_side);
                }
                else {
                    velocity.y *= -1;
                }
            }
            else {
                velocity.y *= -1;
            }
        }
        if (position.y < -60) {
            Portal p = this->cube->getPortal(TOP);
            if (p.dest_cube) {
                if (!p.dest_cube->sprite_set.empty()) {
                    switch_cubes(p.dest_cube, p.dest_side);
                }
                else {
                    velocity.y *= -1;
                }
            }
            else {
                velocity.y *= -1;
            }
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
        for (CubeID cid : cubes_active) {
            cubes[cid].tick(step);
        }
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
                    cubes[cid].init(cid, cubes);
                    cubes_active.mark(cid);
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
    void onNeighborAdd(unsigned c0, unsigned s0, unsigned c1, unsigned s1) 
    {
        if (c0 >= NUM_CUBES || c1 >= NUM_CUBES)
            return;
        cubes[c0].getPortal(s0).open(&cubes[c1], s1);
        cubes[c1].getPortal(s1).open(&cubes[c0], s0);
    }
    void onNeighborRemove(unsigned c0, unsigned s0, unsigned c1, unsigned s1) 
    {
        if (c0 >= NUM_CUBES || c1 >= NUM_CUBES)
            return;

        cubes[c0].getPortal(s0).close();
        cubes[c1].getPortal(s1).close();

    }
    void onRestart() {
        running = false;
    }
};
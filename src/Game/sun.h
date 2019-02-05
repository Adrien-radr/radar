#ifndef SUN_H
#define SUN_H

#include "rf/ui.h"
#include "definitions.h"

namespace game {
    // Keeps a binary state with its previous state value
    struct binary_switch
    {
        binary_switch() : Current(false), Last(false) {}

        void Switch() { Current = !Current; }
        bool Switched() { return Current != Last; }
        void Update() { Last = Current; }
        operator bool() const { return Current; }

        private:
        bool Current;
        bool Last;
    };

    // TODO - divide this with scene, etc
    struct state
    {
        real64 EngineTime;
		real64 dTime;
        camera Camera;
        binary_switch DisableMouse;
        vec4f LightColor;

        real64 WaterCounter;
        real32 WaterStateInterp;
        real32 WaterDirection;
        int    WaterState;

        vec3f  SunDirection;
        real32 SunSpeed;

        real64 Counter;
        real64 CounterTenth;
        bool IsNight;
        // DayPhase: 0 = noon, pi = midnight
        real32 DayPhase;
        // Varies with season
        real32 EarthTilt;
        // Latitude: -pi/2 = South pole, +pi/2 = North pole
        real32 Latitude;

        static int32 const TextlineCapacity = 16;
		int32 TextLineCount;
        rf::ui::text_line Textline[TextlineCapacity];
    };

    bool Init(state *State, config *Config);
    void Destroy(state *State);
    void Update(state *State, rf::input *Input, rf::context *Context);
}

#endif

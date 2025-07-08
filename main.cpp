#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdint.h>
#include <iostream>
#include <stack>
#include <chrono>
#include <fstream>
#include <cstdlib>

int main(){
    uint8_t memory[4096];

    //initialising font
    uint8_t font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, //0
        0x20, 0x60, 0x20, 0x20, 0xF0, //1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
        0x90, 0x90, 0xF0, 0x10, 0x10, //4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
        0xF0, 0x10, 0x20, 0x40, 0x40, //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
        0xF0, 0x90, 0xF0, 0x10, 0x10, //9
        0xF0, 0x90, 0xF0, 0x90, 0x90, //A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
        0xF0, 0x80, 0x80, 0x80, 0xF0, //C
        0xE0, 0x90, 0x90, 0x90, 0xE0, //D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
        0xF0, 0x80, 0xF0, 0x80, 0x80  //F
    };
    for(int x=0; x < 80; x++){
        memory[0x050 + x] = font[x];
    };


    //initialise starting variables
    uint16_t pc = 0x200;
    uint16_t I = 0;
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;
    uint8_t V[16];
    std::fill(std::begin(V), std::end(V), 0);
    bool disp[32][64];
    for(int y=0;y<32;y++){
        for(int x=0;x<64;x++){
            disp[y][x]=0;
        }
    }
    bool keypad[16];
    std::fill(std::begin(keypad), std::end(keypad), false);
    std::stack<uint16_t> st;
    auto last_timer_tick = std::chrono::high_resolution_clock::now();
    auto last_cycle_time = std::chrono::high_resolution_clock::now();


    //loading rom
    std::ifstream rom("pong.ch8", std::ios::binary | std::ios::ate);
    std::streamsize size = rom.tellg();
    rom.seekg(0, std::ios::beg);
    rom.read(reinterpret_cast<char*>(&memory[0x200]), size);

    //initialise display
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;

    while(running){
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool pressed = (e.type == SDL_KEYDOWN);
                switch(e.key.keysym.sym){
                    case SDLK_1:
                        keypad[0x1] = pressed;
                        break;
                    case SDLK_2:
                        keypad[0x2] = pressed;
                        break;
                    case SDLK_3:
                        keypad[0x3] = pressed;
                        break;
                    case SDLK_4:
                        keypad[0xC] = pressed;
                        break;
                    case SDLK_q:
                        keypad[0x4] = pressed;
                        break;
                    case SDLK_w:
                        keypad[0x5] = pressed;
                        break;
                    case SDLK_e:
                        keypad[0x6] = pressed;
                        break;
                    case SDLK_r:
                        keypad[0xD] = pressed;
                        break;
                    case SDLK_a:
                        keypad[0x7] = pressed;
                        break;
                    case SDLK_s:
                        keypad[0x8] = pressed;
                        break;
                    case SDLK_d:
                        keypad[0x9] = pressed;
                        break;
                    case SDLK_f:
                        keypad[0xE] = pressed;
                        break;
                    case SDLK_z:
                        keypad[0xA] = pressed;
                        break;
                    case SDLK_x:
                        keypad[0x0] = pressed;
                        break;
                    case SDLK_c:
                        keypad[0xB] = pressed;
                        break;
                    case SDLK_v:
                        keypad[0xF] = pressed;
                        break;
                }
            }
            else if (e.type == SDL_QUIT) {
                running = false;
            }
        }
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_timer_tick);

        if(elapsed.count() >= 1000 / 60){
            if(delay_timer > 0){
                delay_timer--;
            }
            if(sound_timer > 0){
                sound_timer--;
            }
            last_timer_tick = now;
        }
        auto elapsed_cycle = std::chrono::duration_cast<std::chrono::microseconds>(now - last_cycle_time);
        if (elapsed_cycle.count() >= 1428) {
            uint16_t opcode = (memory[pc] << 8) | memory[pc+1];
            pc += 2;
            uint8_t X = (opcode & 0x0F00) >> 8; //use to check values in V
            uint8_t Y = (opcode & 0x00F0) >> 4; //use to check values in V
            uint8_t N = (opcode & 0x000F);
            uint8_t NN = (opcode & 0x00FF);
            uint16_t NNN = (opcode & 0x0FFF);
            switch(opcode & 0xF000){
                case 0x0000: {
                    if(opcode == 0x00E0){
                        for(int y=0;y<32;y++){
                            for(int x=0;x<64;x++){
                                disp[y][x]=0;
                            }
                        }
                    }
                    else if(opcode == 0x00EE){
                        if (!st.empty()) {
                            pc = st.top();
                            st.pop();
                        } 
                        else{
                            running = false;
                        }
                    }
                    break; 
                }
                case 0x1000: {
                    pc = NNN;
                    break;
                }
                case 0x2000: {
                    st.push(pc);
                    pc = NNN;
                    break;
                }
                case 0x3000: {
                    if(V[X] == NN){
                        pc += 2;
                    }
                    break;
                }
                case 0x4000: {
                    if(V[X] != NN){
                        pc += 2;
                    }
                    break;
                }
                case 0x5000: {
                    if(V[X] == V[Y]){
                        pc += 2;
                    }
                    break;
                }
                case 0x6000: {
                    V[X] = NN;
                    break;
                }
                case 0x7000: {
                    V[X] += NN;
                    break;
                }
                case 0x8000: {
                    switch(N){
                        case 0x0: {
                            V[X] = V[Y];
                            break;
                        }
                        case 0x1: {
                            V[X] |= V[Y];
                            break;
                        }
                        case 0x2: {
                            V[X] &= V[Y];
                            break;
                        }
                        case 0x3: {
                            V[X] ^= V[Y];
                            break;
                        }
                        case 0x4: {
                            uint16_t sum = V[X] + V[Y];
                            V[0xF] = (sum > 255) ? 1 : 0;
                            V[X] = sum & 0xFF;
                            break;
                        }
                        case 0x5: {
                            V[0xF] = (V[X] >= V[Y]) ? 1 : 0;
                            V[X] = V[X] - V[Y];
                            break;
                        }
                        case 0x6: {
                            V[0xF] = V[X] & 0x1;
                            V[X] >>= 1;
                            break;
                        }
                        case 0x7: {
                            V[0xF] = (V[Y] >= V[X]) ? 1 : 0;
                            V[X] = V[Y] - V[X];
                            break;
                        }
                        case 0xE: {
                            V[0xF] = (V[X] & 0x80) >> 7;
                            V[X] <<= 1;
                            break;
                        }
                    }
                    break;
                }
                case 0x9000: {
                    if(V[X] != V[Y]){
                        pc += 2;
                    }
                    break;
                }
                case 0xA000: {
                    I = NNN;
                    break;
                }
                case 0xB000: {
                    pc = NNN + V[0];
                    break;
                }
                case 0xC000: {
                    V[X] = rand() & NN;
                    break;
                }
                case 0xD000: {
                    uint8_t drawx = V[X];
                    uint8_t drawy = V[Y];
                    uint8_t height = N;
                    V[0xF] = 0;
                    for(int row = 0; row < height; row++){
                        uint8_t spriteByte = memory[I + row];
                        for(int bit = 0; bit < 8; bit++){
                            uint8_t pixel = (spriteByte >> (7 - bit)) & 0x1;
                            uint8_t px = drawx + bit;
                            uint8_t py = drawy + row;
                            if (px >= 64 || py >= 32){
                                continue;
                            }
                            if (disp[py][px] == 1 && pixel == 1) {
                                V[0xF] = 1;
                            }
                            disp[py][px] ^= pixel;
                        }
                    }
                    break;
                }
                case 0xE000: {
                    switch(NN){
                        case 0xA1: {
                            if(keypad[V[X]] != true){
                                pc += 2;
                            }
                            break;
                        }
                        case 0x9E:{
                            if(keypad[V[X]] == true){
                                pc += 2;
                            }
                            break;
                        }
                    }
                    break;
                }
                case 0xF000: {
                    switch(NN){
                        case 0x07: {
                            V[X] = delay_timer;
                            break;
                        }
                        case 0x0A: {
                            bool found = false;
                            for(int x=0; x < 16; x++){
                                if(keypad[x]){
                                    V[X] = x;
                                    found = true;
                                    break;
                                }
                            }
                            if(!found){
                                pc -= 2;
                            }
                            break;
                        }
                        case 0x15: {
                            delay_timer = V[X];
                            break;
                        }
                        case 0x18:{
                            sound_timer = V[X];
                            break;
                        }
                        case 0x1E:{
                            I += V[X];
                            V[0xF] = (I > 0x0FFF) ? 1 : 0;
                            I = I & 0x0FFF;
                            break;
                        }
                        case 0x29:{
                            I = 0x050 + (V[X] * 5);
                            break;
                        }
                        case 0x33:{
                            memory[I+2] = V[X]%10;
                            memory[I+1] = V[X]/10%10;
                            memory[I] = V[X]/100%10;
                            break;
                        }
                        case 0x55:{
                            for(int x=0; x<=X; x++){
                                memory[I+x] = V[x];
                            }
                            break;
                        }
                        case 0x65:{
                            for(int x=0; x<=X; x++){
                                V[x] = memory[I+x];
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            last_cycle_time = now;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (disp[y][x]) {
                    SDL_Rect pixel = { x * 10, y * 10, 10, 10 }; // scale by 10x
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

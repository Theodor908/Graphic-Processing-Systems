#include "display/game_window.hpp"

int main() {
    // Create the game window with the specified size and title
    GameWindow gw = GameWindow{ 800, 600, "graphics-processing-systems" };
    return gw.Run();
}
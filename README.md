# SlimeProject

SlimeProject is a game development project consisting of two main components: SlimeOdyssey (a game engine library) and SlimeGame (a game using the SlimeOdyssey engine).

![image](https://github.com/user-attachments/assets/be0031a9-1443-4b8e-a600-4aa9e0f7ca75)


## Project Structure

```
SlimeProject/
├── SlimeOdyssey/  # Game engine library
├── SlimeGame/     # Game implementation
└── CMakeLists.txt # Top-level CMake file
```

## Prerequisites

- CMake (version 3.20 or higher)
- C++20 compatible compiler
- Vulkan SDK

## Building the Project

1. Clone the repository:
   ```
   git clone https://github.com/AlexMollard/SlimeOdyssey.git
   ```
   
2. Navigate to the project directory:
   ```
   cd SlimeOdyssey
   ```

2. Build the project:
   ```
   cmake -S .  -B build
   ```

## Running the Game

After building, you can find the SlimeGame executable in the `build/bin` directory.

## Development

- `SlimeOdyssey/`: Contains the game engine library code.
- `SlimeGame/`: Contains the game-specific code using the SlimeOdyssey engine.

To make changes, edit the files in these directories and rebuild the project.

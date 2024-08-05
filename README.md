# SlimeProject

SlimeProject is a game development project consisting of two main components: SlimeOdyssey (a game engine library) and SlimeGame (a game using the SlimeOdyssey engine).
![image](https://github.com/user-attachments/assets/4f4498c2-1deb-4c52-8ac2-a8515645774d)
![image](https://github.com/user-attachments/assets/229d0dc7-eb05-4651-8d49-2df8f8564e9a)

## Project Structure

```
SlimeProject/
├── SlimeOdyssey/  # Game engine library
├── SlimeGame/     # Game implementation
├── SlimeTests/    # Unit Testing
└── CMakeLists.txt # Top-level CMake file
```

## Prerequisites

- CMake (version 3.20 or higher)
- C++20 compatible compiler
- Vulkan SDK `1.3.2.283.0`

## Building the Project

1. Clone the repository:
   ```
   git clone https://github.com/AlexMollard/SlimeOdyssey.git
   ```

2. Navigate to the project directory:
   ```
   cd SlimeProject
   ```

2. Configure the project:
   ```
   cmake -S . -B build -DENABLE_TESTING=OFF
   ```

3. Build the project
   ```
   cmake --build Build
   ```

## Running the Game

After building, you can find the SlimeGame executable in the `build/bin` directory.

## Development

- `SlimeOdyssey/`: Contains the game engine library code.
- `SlimeGame/`: Contains the game-specific code using the SlimeOdyssey engine.
- `SlimeTets`: Contains Unit tests.

To make changes, edit the files in these directories and rebuild the project.

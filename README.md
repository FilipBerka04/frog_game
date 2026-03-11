# Jumping Frog Game

## Overview
This project is a jumping frog game developed for a Basics of Programming university course. The game is written entirely in C and relies on the `ncurses` library for terminal-based graphics, meaning it works only on Linux environments. The objective is to navigate the player across a map containing grass and roads, avoiding hazards to reach the finish line.

## Implementation Details
* **Mechanics**: The game map is composed of different lane types (Road, Grass, Finish) and features various hazards, including obstacles and cars. Cars have diverse behaviors, such as moving left or right, varying speeds, and either wrapping around the screen or disappearing at the borders.
* **Entities**: The program manages a player character, "bad" cars that kill the player on impact, "good" cars that the player can safely ride in, and a stork that actively chases the player on higher levels.
* **Configuration**: Core game settings are highly customizable via a `CONFIG.txt` file, allowing adjustments to map dimensions (e.g., `MAP_HEIGHT`, `MAP_WIDTH`), entity delays, road numbers, and obstacle density.
* **Progression and Scoring**: The game consists of multiple levels of increasing difficulty. It calculates the player's score based on completion time and number of moves, and maintains a top-10 leaderboard saved to a local `SCORES.txt` file.

## Constraints and System Requirements
* The game requires a Linux environment and the `ncurses` library to compile and run.
* The terminal window must meet minimum size requirements depending on the map dimensions defined in the configuration; otherwise, the program will throw a configuration error.

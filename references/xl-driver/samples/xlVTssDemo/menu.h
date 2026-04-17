#pragma once

#include <Windows.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <string>

#define MENU_DIVIDER "----------------------------------------------------------"

enum class MenuAction {
  EXIT,
  CONTINUE,
  SHOW_HELP,
};

using MenuPrinter = void (*)();
using MenuHandler = MenuAction (*)(char choice);


inline void printMenuHeader(const std::string& name) {
  std::cout << "\n" MENU_DIVIDER "\n";
  constexpr int     width        = static_cast<int>(sizeof(MENU_DIVIDER) - 5);
  const int         nameLength   = static_cast<int>(name.length());
  const std::size_t paddingStart = static_cast<std::size_t>(std::max(width / 2 - nameLength / 2, 0));
  const std::size_t paddingEnd   = static_cast<std::size_t>(std::max(width - nameLength - static_cast<int>(paddingStart), 0));
  std::cout << "- " << std::string(paddingStart, ' ') << name << std::string(paddingEnd, ' ') << " -";
  std::cout << "\n" MENU_DIVIDER "\n";
}

inline void menuMain(const std::string& name, MenuPrinter printer, MenuHandler handler) {
  MenuAction   action      = MenuAction::SHOW_HELP;
  const HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
  bool firstLoop = true;
  // Main menu loop.
  while (action != MenuAction::EXIT) {
    if (action == MenuAction::SHOW_HELP) {
      printMenuHeader(name);
      printer();
      std::cout << "- 'h'\tPrint this menu\n";
      std::cout << "- 'q'\tExit " << name << "\n";
      std::cout << "- '#'\tClear the screen and show the menu\n";
      std::cout << "- 'ESC'\tExit the application\n" MENU_DIVIDER "\n\n";
      action = MenuAction::CONTINUE;
    }
    if (firstLoop) {
      handler('H');
      firstLoop = false;
    }
    // Get console input.
    unsigned long n = 0;
    INPUT_RECORD  ir{};
    ReadConsoleInput(inputHandle, &ir, 1, &n);
    if (n == 1 && ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown == TRUE) {
      // Quick exit via Escape key.
      if (ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) {
        std::exit(EXIT_SUCCESS);
      }

      const char choice = static_cast<char>(ir.Event.KeyEvent.uChar.AsciiChar);
      switch (choice) {
        case 'q':
          action = MenuAction::EXIT;
          break;
        case 'h':
          action = MenuAction::SHOW_HELP;
          break;
        case '#':
          system("cls");
          action = MenuAction::SHOW_HELP;
          break;
        default:
          action = handler(choice);
          break;
      }
    }
  }
}

enum class WindowsColor : WORD {};

constexpr WindowsColor colored(WORD color) {
  return static_cast<WindowsColor>(color);
}

constexpr WindowsColor resetColor = colored(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

inline std::ostream& operator<<(std::ostream& stream, WindowsColor color) {
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
  return stream;
}
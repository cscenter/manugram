#include <iostream>
#include <memory>
#include "model.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

Model model;

void show_all() {
  for (PFigure f : model) {
    cout << f << ": " << f->str() << endl;
  }
}

void add() {
  string type;
  cin >> type;
  if (type == "segment") {
    double x1, y1, x2, y2;
    cin >> x1 >> y1 >> x2 >> y2;
    model.addFigure(std::make_shared<figures::Segment>(Point(x1, y1), Point(x2, y2)));
  } else {
    cout << "Unknown object type: " << type << endl;
  }
}

void del() {
  void *id;
  cin >> id;

  for (auto it = model.begin(); it != model.end(); it++) {
    if (it->get() == id) {
      cout << "Successfully deleted " << (*it)->str() << endl;
      model.removeFigure(it);
      return;
    }
  }
  cout << "Object not found" << endl;
}

void help() {
  cout <<
  "Commands:\n"
  "  show_all\n"
  "  add segment <x1> <y1> <x2> <y2>\n"
  "  del <id>\n"
  "  help\n"
  "  quit\n"
  ;
}

int main() {
  string command;
  for (;;) {
    cout << "> ";
    if (!(cin >> command)) break;

    if (command == "show_all") { 
      show_all();
    } else if (command == "add") { 
      add();
    } else if (command == "del") { 
      del();
    } else if (command == "help") {
      help();
    } else if (command == "quit") {
      break;
    } else {
      cout << "Unknown command: " << command << endl;
    }

    std::string rest;
    getline(cin, rest);
  }
  return 0;
}

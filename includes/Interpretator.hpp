
#ifndef INTERPRETATOR_H
#define INTERPRETATOR_H

#include "ChangeCreator.hpp"
#include "Message.hpp"
#include "Mode.hpp"
#include "Position.hpp"
#include <string>

class Interpretator {
public:
  Interpretator() : changeCreator(), buffer() {}

  Interpretator(const ChangeCreator &_changeCreator)
      : changeCreator(_changeCreator), buffer() {}

  Change Interpret(char &curChar, const Mode &mode, const Position &position);

private:
  ChangeCreator changeCreator;
  std::string buffer;
};

#endif //  INTERPRETATOR_H
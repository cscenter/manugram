#ifndef MODEL_IO_H
#define MODEL_IO_H

#include <iostream>
#include "model.h"
class model_format_error : std::runtime_error {
public:
    model_format_error(const std::string &message) : std::runtime_error("Invalid model format: " + message) {}
    virtual const char *what() const throw() { return std::runtime_error::what(); }
};

std::istream &operator>>(std::istream &in , Model &model);
std::ostream &operator<<(std::ostream &out , Model &model);
void exportModelToSvg(Model &m, std::ostream &out);

#endif // MODEL_IO_H

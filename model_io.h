#ifndef MODEL_IO_H
#define MODEL_IO_H

#include "model.h"
#include <QString>
class io_error : std::runtime_error {
public:
    io_error(const std::string &message) : std::runtime_error(message) {}
    virtual const char *what() const throw() { return std::runtime_error::what(); }
};

class model_format_error : public io_error {
public:
    model_format_error(const std::string &message) : io_error("Invalid model format: " + message) {}
};

std::istream &operator>>(std::istream &in , Model &model);
std::ostream &operator<<(std::ostream &out , Model &model);
void exportModelToSvg(Model &m, std::ostream &out);
void exportModelToTikz(Model &m, std::ostream &out);
void exportModelToImageFile(Model &model, const QString &filename);

std::istream &operator>>(std::istream &in ,       Track &track);
std::ostream &operator<<(std::ostream &out, const Track &track);

#endif // MODEL_IO_H

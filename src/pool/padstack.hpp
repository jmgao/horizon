#pragma once
#include "common/common.hpp"
#include "common/hole.hpp"
#include "common/layer_provider.hpp"
#include "common/lut.hpp"
#include "common/polygon.hpp"
#include "common/shape.hpp"
#include "nlohmann/json_fwd.hpp"
#include "parameter/program.hpp"
#include "parameter/program_polygon.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include <fstream>
#include <map>
#include <set>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class Padstack : public UUIDProvider, public LayerProvider {
public:
    class MyParameterProgram : public ParameterProgramPolygon {
        friend Padstack;

    protected:
        std::map<UUID, Polygon> &get_polygons() override;

    private:
        ParameterProgram::CommandHandler get_command(const std::string &cmd) override;
        class Padstack *ps = nullptr;

        std::pair<bool, std::string> set_shape(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack);
        std::pair<bool, std::string> set_hole(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack);

    public:
        MyParameterProgram(class Padstack *p, const std::string &code);
    };

    enum class Type { TOP, BOTTOM, THROUGH, VIA, HOLE, MECHANICAL };
    static const LutEnumStr<Padstack::Type> type_lut;

    Padstack(const UUID &uu, const json &j);
    Padstack(const UUID &uu);
    static Padstack new_from_file(const std::string &filename);

    json serialize() const;

    Padstack(const Padstack &ps);
    void operator=(Padstack const &ps);

    UUID uuid;
    std::string name;
    std::string well_known_name;
    Type type = Type::TOP;
    std::map<UUID, Polygon> polygons;
    std::map<UUID, Hole> holes;
    std::map<UUID, Shape> shapes;

    ParameterDef parameter_def;
    ParameterSet parameter_set;
    MyParameterProgram parameter_program;
    std::pair<bool, std::string> apply_parameter_set(const ParameterSet &ps);

    UUID get_uuid() const override;
    std::pair<Coordi, Coordi> get_bbox(bool copper_only = false) const;
    void expand_inner(unsigned int n_inner);
    const std::map<int, Layer> &get_layers() const override;

private:
    void update_refs();
};
} // namespace horizon

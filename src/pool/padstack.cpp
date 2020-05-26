#include "padstack.hpp"
#include "board/board_layers.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<Padstack::Type> Padstack::type_lut = {
        {"top", Padstack::Type::TOP}, {"bottom", Padstack::Type::BOTTOM}, {"through", Padstack::Type::THROUGH},
        {"via", Padstack::Type::VIA}, {"hole", Padstack::Type::HOLE},     {"mechanical", Padstack::Type::MECHANICAL}};

Padstack::MyParameterProgram::MyParameterProgram(Padstack *p, const std::string &c) : ParameterProgramPolygon(c), ps(p)
{
}

std::pair<bool, std::string> Padstack::MyParameterProgram::set_shape(const ParameterProgram::TokenCommand *cmd,
                                                                     std::deque<int64_t> &stack)
{
    if (cmd->arguments.size() < 2 || cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR
        || cmd->arguments.at(1)->type != ParameterProgram::Token::Type::STR)
        return {true, "not enough arguments"};

    auto pclass = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(0).get())->string;
    auto form = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(1).get())->string;

    if (form == "rectangle") {
        int64_t width, height;
        if (stack_pop(stack, height) || stack_pop(stack, width))
            return {true, "empty stack"};
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::RECTANGLE;
                it.second.params = {width, height};
            }
        }
    }
    else if (form == "circle") {
        int64_t diameter;
        if (stack_pop(stack, diameter))
            return {true, "empty stack"};
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::CIRCLE;
                it.second.params = {diameter};
            }
        }
    }
    else if (form == "obround") {
        int64_t width, height;
        if (stack_pop(stack, height) || stack_pop(stack, width))
            return {true, "empty stack"};
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.form = Shape::Form::OBROUND;
                it.second.params = {width, height};
            }
        }
    }
    else if (form == "position") {
        int64_t x, y;
        if (stack_pop(stack, y) || stack_pop(stack, x))
            return {true, "empty stack"};
        for (auto &it : ps->shapes) {
            if (it.second.parameter_class == pclass) {
                it.second.placement.shift = {x, y};
            }
        }
    }

    else {
        return {true, "unknown form " + form};
    }

    return {false, ""};
}

std::pair<bool, std::string> Padstack::MyParameterProgram::set_hole(const ParameterProgram::TokenCommand *cmd,
                                                                    std::deque<int64_t> &stack)
{
    if (cmd->arguments.size() < 2 || cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR
        || cmd->arguments.at(1)->type != ParameterProgram::Token::Type::STR)
        return {true, "not enough arguments"};

    auto pclass = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(0).get())->string;
    auto shape = dynamic_cast<ParameterProgram::TokenString *>(cmd->arguments.at(1).get())->string;

    if (shape == "round") {
        int64_t diameter;
        if (stack_pop(stack, diameter))
            return {true, "empty stack"};
        for (auto &it : ps->holes) {
            if (it.second.parameter_class == pclass) {
                it.second.shape = Hole::Shape::ROUND;
                it.second.diameter = diameter;
            }
        }
    }
    else if (shape == "slot") {
        int64_t diameter, length;
        if (stack_pop(stack, length) || stack_pop(stack, diameter))
            return {true, "empty stack"};
        for (auto &it : ps->holes) {
            if (it.second.parameter_class == pclass) {
                it.second.shape = Hole::Shape::SLOT;
                it.second.diameter = diameter;
                it.second.length = length;
            }
        }
    }

    else {
        return {true, "unknown shape " + shape};
    }

    return {false, ""};
}

std::map<UUID, Polygon> &Padstack::MyParameterProgram::get_polygons()
{
    return ps->polygons;
}


ParameterProgram::CommandHandler Padstack::MyParameterProgram::get_command(const std::string &cmd)
{
    using namespace std::placeholders;
    if (auto r = ParameterProgram::get_command(cmd)) {
        return r;
    }
    else if (cmd == "set-shape") {
        return std::bind(std::mem_fn(&Padstack::MyParameterProgram::set_shape), this, _1, _2);
    }
    else if (cmd == "set-hole") {
        return std::bind(std::mem_fn(&Padstack::MyParameterProgram::set_hole), this, _1, _2);
    }
    else if (cmd == "set-polygon") {
        return std::bind(std::mem_fn(&Padstack::MyParameterProgram::set_polygon), this, _1, _2);
    }
    else if (cmd == "set-polygon-vertices") {
        return std::bind(std::mem_fn(&Padstack::MyParameterProgram::set_polygon_vertices), this, _1, _2);
    }
    else if (cmd == "expand-polygon") {
        return std::bind(std::mem_fn(&Padstack::MyParameterProgram::expand_polygon), this, _1, _2);
    }
    return nullptr;
}

Padstack::Padstack(const Padstack &ps)
    : uuid(ps.uuid), name(ps.name), type(ps.type), polygons(ps.polygons), holes(ps.holes), shapes(ps.shapes),
      parameter_def(ps.parameter_def), parameter_set(ps.parameter_set), parameter_program(ps.parameter_program)
{
    update_refs();
}

void Padstack::operator=(Padstack const &ps)
{
    uuid = ps.uuid;
    name = ps.name;
    type = ps.type;
    polygons = ps.polygons;
    holes = ps.holes;
    shapes = ps.shapes;
    parameter_set = ps.parameter_set;
    parameter_program = ps.parameter_program;
    update_refs();
}

void Padstack::update_refs()
{
    parameter_program.ps = this;
}

Padstack::Padstack(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), well_known_name(j.value("well_known_name", "")),
      parameter_program(this, j.value("parameter_program", ""))
{
    {
        const json &o = j["polygons"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            polygons.emplace(std::make_pair(u, Polygon(u, it.value())));
        }
    }
    map_erase_if(polygons, [](const auto &a) { return a.second.vertices.size() == 0; });
    {
        const json &o = j["holes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            holes.emplace(std::make_pair(u, Hole(u, it.value())));
        }
    }
    if (j.count("shapes")) {
        const json &o = j["shapes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            shapes.emplace(std::make_pair(u, Shape(u, it.value())));
        }
    }
    if (j.count("padstack_type")) {
        type = type_lut.lookup(j.at("padstack_type"));
    }
    if (j.count("parameter_set")) {
        parameter_set = parameter_set_from_json(j.at("parameter_set"));
    }

    parameter_def = ParameterDef::deserialize(j);
} // namespace horizon

Padstack Padstack::new_from_file(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    return Padstack(UUID(j.at("uuid").get<std::string>()), j);
}

Padstack::Padstack(const UUID &uu) : uuid(uu), parameter_program(this, "")
{
}

json Padstack::serialize() const
{
    json j;
    j["uuid"] = (std::string)uuid;
    j["type"] = "padstack";
    j["name"] = name;
    j["well_known_name"] = well_known_name;
    j["padstack_type"] = type_lut.lookup_reverse(type);
    j["parameter_program"] = parameter_program.get_code();
    parameter_def.serialize(j);
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    j["polygons"] = json::object();
    for (const auto &it : polygons) {
        j["polygons"][(std::string)it.first] = it.second.serialize();
    }
    j["holes"] = json::object();
    for (const auto &it : holes) {
        j["holes"][(std::string)it.first] = it.second.serialize();
    }
    j["shapes"] = json::object();
    for (const auto &it : shapes) {
        j["shapes"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}

UUID Padstack::get_uuid() const
{
    return uuid;
}

std::pair<Coordi, Coordi> Padstack::get_bbox(bool copper_only) const
{
    Coordi a;
    Coordi b;
    for (const auto &it : polygons) {
        if (!copper_only || BoardLayers::is_copper(it.second.layer)) {
            auto poly = it.second.remove_arcs(8);
            for (const auto &v : poly.vertices) {
                a = Coordi::min(a, v.position);
                b = Coordi::max(b, v.position);
            }
        }
    }
    for (const auto &it : shapes) {
        if (!copper_only || BoardLayers::is_copper(it.second.layer)) {
            auto bb = it.second.placement.transform_bb(it.second.get_bbox());

            a = Coordi::min(a, bb.first);
            b = Coordi::max(b, bb.second);
        }
    }

    return std::make_pair(a, b);
}

const std::map<int, Layer> &Padstack::get_layers() const
{
    static const std::map<int, Layer> layers = {{30, {30, "Top Paste"}},
                                                {10, {10, "Top Mask"}},
                                                {0, {0, "Top Copper", false, true}},
                                                {-1, {-1, "Inner", false, true}},
                                                {-100, {-100, "Bottom Copper", true, true}},
                                                {-110, {-110, "Bottom Mask", true}},
                                                {-130, {-130, "Bottom Paste"}}};
    return layers;
}

std::pair<bool, std::string> Padstack::apply_parameter_set(const ParameterSet &ps)
{
    return parameter_program.run(parameter_set_merge(parameter_set, ps));
}

void Padstack::expand_inner(unsigned int n_inner)
{
    if (n_inner == 0) {
        for (auto it = shapes.begin(); it != shapes.end();) {
            if (it->second.layer == -1) {
                shapes.erase(it++);
            }
            else {
                it++;
            }
        }
        for (auto it = polygons.begin(); it != polygons.end();) {
            if (it->second.layer == -1) {
                polygons.erase(it++);
            }
            else {
                it++;
            }
        }
    }
    std::map<UUID, Polygon> new_polygons;
    for (const auto &it : polygons) {
        if (it.second.layer == -1) {
            for (unsigned int i = 0; i < (n_inner - 1); i++) {
                auto uu = UUID::random();
                auto &np = new_polygons.emplace(uu, it.second).first->second;
                np.uuid = uu;
                np.layer = -2 - i;
            }
        }
    }
    polygons.insert(new_polygons.begin(), new_polygons.end());

    std::map<UUID, Shape> new_shapes;
    for (const auto &it : shapes) {
        if (it.second.layer == -1) {
            for (unsigned int i = 0; i < (n_inner - 1); i++) {
                auto uu = UUID::random();
                auto &np = new_shapes.emplace(uu, it.second).first->second;
                np.uuid = uu;
                np.layer = -2 - i;
            }
        }
    }
    shapes.insert(new_shapes.begin(), new_shapes.end());
}
} // namespace horizon

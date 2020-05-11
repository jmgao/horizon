#pragma once
#include "nlohmann/json_fwd.hpp"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace horizon {
using json = nlohmann::json;

#define BUILTIN_PARAMETER_DEFS()                                                                                       \
    X(PAD_WIDTH, "pad_width", "Pad width")                                                                             \
    X(PAD_HEIGHT, "pad_height", "Pad height")                                                                          \
    X(PAD_DIAMETER, "pad_diameter", "Pad diameter")                                                                    \
    X(SOLDER_MASK_EXPANSION, "solder_mask_expansion", "Solder mask expansion")                                         \
    X(PASTE_MASK_CONTRACTION, "paste_mask_contraction", "Paste mask contraction")                                      \
    X(HOLE_DIAMETER, "hole_diameter", "Hole diameter")                                                                 \
    X(HOLE_LENGTH, "hole_length", "Hole length")                                                                       \
    X(COURTYARD_EXPANSION, "courtyard_expansion", "Courtyard expansion")                                               \
    X(VIA_DIAMETER, "via_diameter", "Via diameter")                                                                    \
    X(HOLE_SOLDER_MASK_EXPANSION, "hole_solder_mask_expansion", "Hole solder mask expansion")                          \
    X(VIA_SOLDER_MASK_EXPANSION, "via_solder_mask_expansion", "Via solder mask expansion")                             \
    X(HOLE_ANNULAR_RING, "hole_annular_ring", "Hole annular ring")                                                     \
    X(CORNER_RADIUS, "corner_radius", "Corner radius")

struct ParameterID {
    explicit ParameterID(std::string id) : value(id)
    {
    }

    bool operator==(const ParameterID &rhs) const
    {
        return value == rhs.value;
    }

    bool operator<(const ParameterID &rhs) const
    {
        if (this->is_builtin() != rhs.is_builtin()) {
            return this->is_builtin() < rhs.is_builtin();
        }
        return this->value < rhs.value;
    }

    bool is_builtin() const
    {
#define X(pid, id, name)                                                                                               \
    if (this->value == id) {                                                                                           \
        return true;                                                                                                   \
    }

        BUILTIN_PARAMETER_DEFS()
#undef X

        return false;
    }

    std::string_view id() const
    {
        return value;
    }

    static ParameterID from_string(std::string str)
    {
        return ParameterID{std::move(str)};
    }

    std::string value;
};

struct BuiltinParameter {
#define X(pid, id, name) static ParameterID pid;
    BUILTIN_PARAMETER_DEFS()
#undef X
};

struct ParameterDef {
    std::optional<std::string_view> lookup_name(const ParameterID &pid) const;

    std::vector<ParameterID> parameters() const;

    // Note: unlike many other serialized types, ParameterDef is stored in multiple child
    //       nodes to maintain compatibility with the previous encoding of required parameters.
    void serialize(json &parent) const;
    static ParameterDef deserialize(const json &parent);

    std::set<ParameterID> parameters_required;
    std::map<ParameterID, std::string> custom_parameters;
};

using ParameterSet = std::map<ParameterID, int64_t>;
ParameterSet parameter_set_from_json(const json &j);
json parameter_set_serialize(const ParameterSet &ps);
ParameterSet parameter_set_merge(ParameterSet base, const ParameterSet &modifications);

ParameterID parameter_id_from_string(const std::string &s);

} // namespace horizon

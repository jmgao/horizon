#include "set.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

#define X(pid, id, name) ParameterID BuiltinParameter::pid(id);
BUILTIN_PARAMETER_DEFS()
#undef X

std::optional<std::string_view> ParameterDef::lookup_name(const ParameterID &pid) const
{
#define X(x_pid, x_id, x_name)                                                                                         \
    if (pid.id() == x_id)                                                                                              \
        return x_name;
    BUILTIN_PARAMETER_DEFS()
#undef X

    auto it = this->custom_parameters.find(pid);
    if (it == this->custom_parameters.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ParameterID> ParameterDef::parameters() const
{
    std::vector<ParameterID> result;
#define X(pid, id, name) result.emplace_back(id);
    BUILTIN_PARAMETER_DEFS()
#undef X
    for (auto &[param, _] : custom_parameters) {
        static_cast<void>(_);
        result.push_back(param);
    }
    return result;
}

void ParameterDef::serialize(json &parent) const
{
    json &req = parent["parameters_required"] = json::array();
    for (const auto &it : this->parameters_required) {
        req.push_back(it.id());
    }

    if (!this->custom_parameters.empty()) {
        json &custom = parent["parameters"] = json::object();
        for (auto &[k, v] : this->custom_parameters) {
            std::string id(k.id());
            json &obj = custom[id] = json::object();
            obj["name"] = v;
        }
    }
}

ParameterDef ParameterDef::deserialize(const json &parent)
{
    ParameterDef pd;
    if (parent.count("parameters")) {
        const json &o = parent["parameters"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            std::string id(it.key());
            if (!it.value().count("name")) {
                continue;
            }
            ParameterID pid(std::move(id));
            pd.custom_parameters[pid] = it.value()["name"];
        }
    }
    if (parent.count("parameters_required")) {
        const json &o = parent["parameters_required"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            // TODO: Check if the required parameter is valid?
            std::string id = it.value();
            pd.parameters_required.emplace(ParameterID::from_string(id));
        }
    }
    return pd;
}

json parameter_set_serialize(const ParameterSet &p)
{
    json j = json::object();
    for (const auto &it : p) {
        std::string id(it.first.id());
        j[id] = it.second;
    }
    return j;
}

ParameterSet parameter_set_from_json(const json &j)
{
    ParameterSet ps;
    for (auto it = j.cbegin(); it != j.cend(); ++it) {
        auto k = ParameterID::from_string(it.key());
        ps[k] = it.value();
    }
    return ps;
}

ParameterSet parameter_set_merge(ParameterSet base, const ParameterSet &modifications)
{
    for (auto &[k, v] : modifications) {
        base[k] = v;
    }
    return base;
}

} // namespace horizon

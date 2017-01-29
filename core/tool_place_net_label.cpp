#include "tool_place_net_label.hpp"
#include <iostream>
#include "core_schematic.hpp"

namespace horizon {

	ToolPlaceNetLabel::ToolPlaceNetLabel(Core *c, ToolID tid):ToolPlaceJunction(c, tid) {
		name = "Place Net label";
	}

	bool ToolPlaceNetLabel::can_begin() {
		return core.c;
	}

	void ToolPlaceNetLabel::create_attached() {
		auto uu = UUID::random();
		la = &core.c->get_sheet()->net_labels.emplace(uu, uu).first->second;
		la->orientation = last_orientation;
		la->junction = temp;
	}

	void ToolPlaceNetLabel::delete_attached() {
		if(la) {
			core.c->get_sheet()->net_labels.erase(la->uuid);
			la = nullptr;
		}
	}

	static Orientation transform_orienation(Orientation orientation, bool rotate) {
		Orientation new_orientation;
		if(rotate) {
			switch(orientation) {
				case Orientation::UP:    new_orientation=Orientation::RIGHT; break;
				case Orientation::DOWN:  new_orientation=Orientation::LEFT; break;
				case Orientation::LEFT:  new_orientation=Orientation::UP; break;
				case Orientation::RIGHT: new_orientation=Orientation::DOWN; break;
			}
		}
		else {
			switch(orientation) {
				case Orientation::UP:    new_orientation=Orientation::UP; break;
				case Orientation::DOWN:  new_orientation=Orientation::DOWN; break;
				case Orientation::LEFT:  new_orientation=Orientation::RIGHT; break;
				case Orientation::RIGHT: new_orientation=Orientation::LEFT; break;
			}
		}
		return new_orientation;
	}

	bool ToolPlaceNetLabel::check_line(LineNet *li) {
		if(li->bus)
			return false;
		return true;
	}

	bool ToolPlaceNetLabel::update_attached(const ToolArgs &args) {
		if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					Junction *j = core.r->get_junction(args.target.path.at(0));
					if(j->bus)
						return true;
					la->junction = j;
					create_attached();
					return true;
				}
				else if(args.target.type == ObjectType::SYMBOL_PIN) {
					SchematicSymbol *schsym = core.c->get_schematic_symbol(args.target.path.at(0));
					SymbolPin *pin = &schsym->symbol.pins.at(args.target.path.at(1));
					UUIDPath<2> connpath(schsym->gate->uuid, args.target.path.at(1));
					if(schsym->component->connections.count(connpath) == 0) {
						//sympin not connected
						auto uu = UUID::random();
						auto *line = &core.c->get_sheet()->net_lines.emplace(uu, uu).first->second;
						line->net = core.c->get_schematic()->block->insert_net();
						line->from.connect(schsym, pin);
						line->to.connect(temp);
						schsym->component->connections.emplace(UUIDPath<2>(schsym->gate->uuid, pin->uuid), line->net);

						temp->temp = false;
						temp->net = line->net;
						switch(la->orientation) {
							case Orientation::LEFT: temp->position.x -= 1.25_mm; break;
							case Orientation::RIGHT: temp->position.x += 1.25_mm; break;
							case Orientation::DOWN: temp->position.y -= 1.25_mm; break;
							case Orientation::UP: temp->position.y += 1.25_mm; break;
						}
						create_junction(args.coords);
						create_attached();
					}

					return true;
				}
			}
		}
		else if(args.type == ToolEventType::KEY) {
			if(la) {
				if(args.key == GDK_KEY_r) {
					la->orientation = transform_orienation(la->orientation, true);
					last_orientation = la->orientation;
					return true;
				}
				else if(args.key == GDK_KEY_plus || args.key == GDK_KEY_equal) {
					la->size += 0.5_mm;
					return true;
				}
				else if(args.key == GDK_KEY_minus ) {
					if(la->size > 0.5_mm) {
						la->size -= 0.5_mm;
					}
					return true;
				}

			}
		}
		return false;
	}
}

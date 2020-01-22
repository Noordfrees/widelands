/*
 * Copyright (C) 2002-2019 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "editor/scripting/function.h"

#include "editor/scripting/function_statements.h"

/************************************************************
                  Functions implementation
************************************************************/

FunctionBase::FunctionBase(const std::string& n,
                           const VariableType& c,
                           const VariableType& r,
                           std::list<std::pair<std::string, VariableType>> p)
   : parameters_(p), name_(n), class_(c), returns_(r) {
}
FunctionBase::FunctionBase(const std::string& n)
   : name_(n), class_(VariableTypeID::Nil), returns_(VariableTypeID::Nil) {
}

std::string FunctionBase::header(bool lua) const {
	std::string s = lua ? "function" : descname(returns_);
	s += " " + name_ + "(";
	for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
		if (it != parameters_.begin()) {
			s += ", ";
		}
		if (!lua) {
			s += descname(it->second) + " ";
		}
		s += it->first;
	}
	return s + ")";
}

constexpr uint16_t kCurrentPacketVersionFunction = 1;

void LuaFunction::selftest() const {
	ScriptingObject::selftest();
	for (const FunctionStatement* a : body_) {
		if (!a) {
			throw wexception("nullptr body statement");
		} else if (is_a(FS_Break, a)) {
			throw wexception("break statement outside for or while loop");
		} else if (upcast(const FS_Return, r, a)) {
			// NOCOM This does not check return statements within nested return loops.
			// We should also check whether the function always returns with an
			// explicit return statement unless we return nil.
			if (get_returns().id() == VariableTypeID::Nil) {
				if (r->get_return())
					throw wexception("non-empty return statement in function returning nil");
			} else {
				if (!r->get_return())
					throw wexception("empty return statement in function returning non-nil");
				if (!r->get_return()->type().is_subclass(get_returns()))
					throw wexception("in return statement: %s cannot be casted to %s",
					                 descname(r->get_return()->type()).c_str(),
					                 descname(get_returns()).c_str());
			}
		}
	}
}

void LuaFunction::load(FileRead& fr, Loader& loader) {
	try {
		ScriptingObject::load(fr, loader);
		uint16_t const packet_version = fr.unsigned_16();
		if (packet_version != kCurrentPacketVersionFunction) {
			throw Widelands::UnhandledVersionError(
			   "LuaFunction", packet_version, kCurrentPacketVersionFunction);
		}
		rename(fr.c_string());
		set_returns(VariableType(fr));
		for (size_t n = fr.unsigned_32(); n; --n) {
			const std::string v(fr.c_string());
			parameters_.push_back(std::make_pair(v, VariableType(fr)));
		}
		for (size_t n = fr.unsigned_32(); n; --n) {
			loader.push_back(fr.unsigned_32());
		}
	} catch (const WException& e) {
		throw wexception("editor scripting lua function: %s", e.what());
	}
}

void LuaFunction::load_pointers(const ScriptingLoader& l, Loader& loader) {
	ScriptingObject::load_pointers(l, loader);
	while (!loader.empty()) {
		body_.push_back(&l.get<FunctionStatement>(loader.front()));
		loader.pop_front();
	}
}

void LuaFunction::save(FileWrite& fw) const {
	ScriptingObject::save(fw);
	fw.unsigned_16(kCurrentPacketVersionFunction);
	fw.c_string(get_name().c_str());
	get_returns().write(fw);

	fw.unsigned_32(parameters_.size());
	for (const auto& pair : parameters_) {
		fw.c_string(pair.first.c_str());
		pair.second.write(fw);
	}

	fw.unsigned_32(body_.size());
	for (const auto& f : body_) {
		fw.unsigned_32(f->serial());
	}
}

void LuaFunction::write_lua(int32_t indent, FileWrite& fw) const {
	fw.print_f("\n%s\n", header(true).c_str());
	::write_lua(indent, fw, body_);
}

std::set<uint32_t> LuaFunction::references() const {
	auto set = ScriptingObject::references();
	for (const FunctionStatement* f : body_) {
		set.insert(f->serial());
	}
	return set;
}

// static
void write_lua(int32_t indent, FileWrite& fw, const std::list<FunctionStatement*>& body) {
	for (const auto& f : body) {
		for (int32_t i = 0; i <= indent; ++i) {
			fw.print_f("   ");
		}
		f->write_lua(indent + 1, fw);
		fw.print_f("\n");
	}
	for (int32_t i = 0; i < indent; ++i) {
		fw.print_f("   ");
	}
}

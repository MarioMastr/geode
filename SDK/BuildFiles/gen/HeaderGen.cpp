#include "SharedGen.hpp"
#include <iostream>
#include <set>

using std::set;

namespace format_strings {
    char const* header_start = R"CAC(
#pragma once
#include <HeaderBase.hpp>
)CAC";

	char const* class_predeclare = "struct {class_name};\n";
    // requires: base_classes, class_name
    char const* class_start = R"CAC(
struct {class_name}{base_classes} {{
)CAC";
    
    // requires: static, virtual, return_type, function_name, raw_parameters, const
    char const* function_definition = "\t{static}{virtual}{return_type} {function_name}({raw_arg_types}){const};\n";

    // requires: static, return_type, function_name, raw_parameters, const, class_name, definition
    char const* ool_function_definition = "{return_type} {class_name}::{function_name}({raw_params}){const} {definition}\n";

    char const* structor_definition = "\t{function_name}({raw_arg_types});\n";
    
    // requires: type, member_name, array
    char const* member_definition = "\t{type} {member_name}{array};\n";

    char const* pad_definition = "\tGEODE_PAD({hardcode});\n";

    // requires: hardcode_macro, type, member_name, hardcode
    char const* hardcode_definition = "\tCLASSPARAM({type}, {member_name}, {hardcode});\n";

    char const* class_end = "};\n";
}

void sortClass(Root& r, ClassDefinition* c, set<ClassDefinition*>& looked, vector<ClassDefinition*>& ordered) {
	if (c->name.find("cocos2d::") != string::npos) return; // cocos headers exist already
    if (looked.find(c) == looked.end()) {
    	looked.insert(c);
        for (string j : c->superclasses) {
        	if (j.find("cocos2d::") != string::npos) continue;
        	if (r.classes.count(c->name) == 0) {
		        cacerr("Expected class definition for %s\n", c->name.c_str());
		    }
		    else {
		    	if (r.classes.find(j) == r.classes.end()) {
		    		cacerr("Create class definition for %s\n", j.c_str());
		    	}
		    	else sortClass(r, &r.classes.at(j), looked, ordered);
		    }    
        }
        ordered.push_back(c);
    }
}

int main(int argc, char** argv) {
	set<ClassDefinition*> looked;
	vector<ClassDefinition*> ordered;

    string output(format_strings::header_start);
    Root root = CacShare::init(argc, argv);

    for (auto& [name, c] : root.classes) {
        sortClass(root, &c, looked, ordered);
    }

    for (auto& cp : ordered) {
    	output += fmt::format(format_strings::class_predeclare,
            fmt::arg("class_name", cp->name)
        );
    }

    vector<Function> outofline;

   	for (auto& cp : ordered) {
   		auto& cd = *cp;


        output += fmt::format(format_strings::class_start,
            fmt::arg("class_name", cd.name),
            fmt::arg("base_classes", CacShare::formatBases(cd.superclasses))
        );

        for (auto i : cd.inlines) {
            // printf("inline %s\n", i.inlined.c_str());
        	output += "\t" + i.inlined + "\n";
        }


        for (auto f : cd.functions) {
            if (f.is_defined)
                outofline.push_back(f);

            if (f.binds[CacShare::platform].size() == 0 && !f.is_defined)
                continue; // Function not supported for this platform, skip it
                
        	char const* used_format;
        	switch (f.function_type) {
                case kDestructor:
                case kConstructor:
                    used_format = format_strings::structor_definition;
                    break;
                default:
               		used_format = format_strings::function_definition;
                    break;
            }
        	output += fmt::format(used_format,
                fmt::arg("virtual", f.function_type == kVirtualFunction ? "virtual " : ""),
                fmt::arg("static", f.function_type == kStaticFunction ? "static " : ""),
                fmt::arg("return_type", CacShare::getReturn(f)),
                fmt::arg("function_name", f.name),
                fmt::arg("raw_arg_types", CacShare::formatRawArgTypes(f.args)),
                fmt::arg("const", f.is_const ? " const" : "")
            );
        }
        for (auto m : cd.members) {
            if (m.member_type != kDefault && CacShare::getHardcode(m).size() == 0)
                continue; // Not Implemented on platform

        	char const* used_format;
        	switch (m.member_type) {
                case kDefault:
                	used_format = format_strings::member_definition;
                	break;
                case kHardcode:
                	used_format = format_strings::hardcode_definition;
                	break;
                case kPad:
                	used_format = format_strings::pad_definition;
                	break;
            }
        	output += fmt::format(used_format,
                fmt::arg("type", m.type),
                fmt::arg("member_name", m.name.substr(m.member_type == kHardcode ? 2 : 0, m.name.size())),
                fmt::arg("hardcode", CacShare::getHardcode(m)),
                fmt::arg("array", CacShare::getArray(m.count)) //why is this not tied to member
            );
        }

        output += format_strings::class_end;

        // queued.pop_front();
    }

    for (auto f : outofline) {
        output += fmt::format(format_strings::ool_function_definition,
                fmt::arg("return_type", CacShare::getReturn(f)),
                fmt::arg("function_name", f.name),
                fmt::arg("raw_params", CacShare::formatRawParams(f.args, f.argnames)),
                fmt::arg("const", f.is_const ? " const" : ""),
                fmt::arg("class_name", f.parent_class->name),
                fmt::arg("definition", f.definition)
        );
    }

    CacShare::writeFile(output);
}

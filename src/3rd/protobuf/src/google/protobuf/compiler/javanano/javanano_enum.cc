
#include <map>
#include <string>

#include <google/protobuf/compiler/javanano/javanano_params.h>
#include <google/protobuf/compiler/javanano/javanano_enum.h>
#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor, const Params& params)
  : params_(params), descriptor_(descriptor) {
  for (int i = 0; i < descriptor_->value_count(); i++) {
    const EnumValueDescriptor* value = descriptor_->value(i);
    const EnumValueDescriptor* canonical_value =
      descriptor_->FindValueByNumber(value->number());

    if (value == canonical_value) {
      canonical_values_.push_back(value);
    } else {
      Alias alias;
      alias.value = value;
      alias.canonical_value = canonical_value;
      aliases_.push_back(alias);
    }
  }
}

EnumGenerator::~EnumGenerator() {}

void EnumGenerator::Generate(io::Printer* printer) {
  printer->Print(
      "\n"
      "// enum $classname$\n",
      "classname", descriptor_->name());

  const string classname = RenameJavaKeywords(descriptor_->name());

  // Start of container interface
  // If generating intdefs, we use the container interface as the intdef if
  // present. Otherwise, we just make an empty @interface parallel to the
  // constants.
  bool use_intdef = params_.generate_intdefs();
  bool use_shell_class = params_.java_enum_style();
  if (use_intdef) {
    // @IntDef annotation so tools can enforce correctness
    // Annotations will be discarded by the compiler
    printer->Print("@java.lang.annotation.Retention("
      "java.lang.annotation.RetentionPolicy.SOURCE)\n"
      "@android.support.annotation.IntDef({\n");
    printer->Indent();
    for (int i = 0; i < canonical_values_.size(); i++) {
      const string constant_name =
          RenameJavaKeywords(canonical_values_[i]->name());
      if (use_shell_class) {
        printer->Print("$classname$.$name$,\n",
          "classname", classname,
          "name", constant_name);
      } else {
        printer->Print("$name$,\n", "name", constant_name);
      }
    }
    printer->Outdent();
    printer->Print("})\n");
  }
  if (use_shell_class || use_intdef) {
    printer->Print(
      "public $at_for_intdef$interface $classname$ {\n",
      "classname", classname,
      "at_for_intdef", use_intdef ? "@" : "");
    if (use_shell_class) {
        printer->Indent();
    } else {
        printer->Print("}\n\n");
    }
  }

  // Canonical values
  for (int i = 0; i < canonical_values_.size(); i++) {
    printer->Print(
      "public static final int $name$ = $canonical_value$;\n",
      "name", RenameJavaKeywords(canonical_values_[i]->name()),
      "canonical_value", SimpleItoa(canonical_values_[i]->number()));
  }

  // Aliases
  for (int i = 0; i < aliases_.size(); i++) {
    printer->Print(
      "public static final int $name$ = $canonical_name$;\n",
      "name", RenameJavaKeywords(aliases_[i].value->name()),
      "canonical_name", RenameJavaKeywords(aliases_[i].canonical_value->name()));
  }

  // End of container interface
  if (use_shell_class) {
    printer->Outdent();
    printer->Print("}\n");
  }
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
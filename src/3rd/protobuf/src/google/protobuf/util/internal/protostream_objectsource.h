
#ifndef GOOGLE_PROTOBUF_UTIL_CONVERTER_PROTOSTREAM_OBJECTSOURCE_H__
#define GOOGLE_PROTOBUF_UTIL_CONVERTER_PROTOSTREAM_OBJECTSOURCE_H__

#include <functional>
#include <google/protobuf/stubs/hash.h>
#include <string>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/type.pb.h>
#include <google/protobuf/util/internal/object_source.h>
#include <google/protobuf/util/internal/object_writer.h>
#include <google/protobuf/util/internal/type_info.h>
#include <google/protobuf/util/type_resolver.h>
#include <google/protobuf/stubs/stringpiece.h>
#include <google/protobuf/stubs/status.h>
#include <google/protobuf/stubs/statusor.h>


namespace google {
namespace protobuf {
class Field;
class Type;
}  // namespace protobuf


namespace protobuf {
namespace util {
namespace converter {

class TypeInfo;

// An ObjectSource that can parse a stream of bytes as a protocol buffer.
// Its WriteTo() method can be given an ObjectWriter.
// This implementation uses a google.protobuf.Type for tag and name lookup.
// The field names are converted into lower camel-case when writing to the
// ObjectWriter.
//
// Sample usage: (suppose input is: string proto)
//   ArrayInputStream arr_stream(proto.data(), proto.size());
//   CodedInputStream in_stream(&arr_stream);
//   ProtoStreamObjectSource os(&in_stream, /*ServiceTypeInfo*/ typeinfo,
//                              <your message google::protobuf::Type>);
//
//   Status status = os.WriteTo(<some ObjectWriter>);
class LIBPROTOBUF_EXPORT ProtoStreamObjectSource : public ObjectSource {
 public:
  ProtoStreamObjectSource(google::protobuf::io::CodedInputStream* stream,
                          TypeResolver* type_resolver,
                          const google::protobuf::Type& type);

  virtual ~ProtoStreamObjectSource();

  virtual util::Status NamedWriteTo(StringPiece name, ObjectWriter* ow) const;

  // Sets whether or not to use lowerCamelCase casing for enum values. If set to
  // false, enum values are output without any case conversions.
  //
  // For example, if we have an enum:
  // enum Type {
  //   ACTION_AND_ADVENTURE = 1;
  // }
  // Type type = 20;
  //
  // And this option is set to true. Then the rendered "type" field will have
  // the string "actionAndAdventure".
  // {
  //   ...
  //   "type": "actionAndAdventure",
  //   ...
  // }
  //
  // If set to false, the rendered "type" field will have the string
  // "ACTION_AND_ADVENTURE".
  // {
  //   ...
  //   "type": "ACTION_AND_ADVENTURE",
  //   ...
  // }
  void set_use_lower_camel_for_enums(bool value) {
    use_lower_camel_for_enums_ = value;
  }

  // Sets the max recursion depth of proto message to be deserialized. Proto
  // messages over this depth will fail to be deserialized.
  // Default value is 64.
  void set_max_recursion_depth(int max_depth) {
    max_recursion_depth_ = max_depth;
  }


 protected:
  // Writes a proto2 Message to the ObjectWriter. When the given end_tag is
  // found this method will complete, allowing it to be used for parsing both
  // nested messages (end with 0) and nested groups (end with group end tag).
  // The include_start_and_end parameter allows this method to be called when
  // already inside of an object, and skip calling StartObject and EndObject.
  virtual util::Status WriteMessage(const google::protobuf::Type& descriptor,
                                      StringPiece name, const uint32 end_tag,
                                      bool include_start_and_end,
                                      ObjectWriter* ow) const;

 private:
  ProtoStreamObjectSource(google::protobuf::io::CodedInputStream* stream,
                          const TypeInfo* typeinfo,
                          const google::protobuf::Type& type);
  // Function that renders a well known type with a modified behavior.
  typedef util::Status (*TypeRenderer)(const ProtoStreamObjectSource*,
                                         const google::protobuf::Type&,
                                         StringPiece, ObjectWriter*);

  // Looks up a field and verify its consistency with wire type in tag.
  const google::protobuf::Field* FindAndVerifyField(
      const google::protobuf::Type& type, uint32 tag) const;

  // TODO(skarvaje): Mark these methods as non-const as they modify internal
  // state (stream_).
  //
  // Renders a repeating field (packed or unpacked).
  // Returns the next tag after reading all sequential repeating elements. The
  // caller should use this tag before reading more tags from the stream.
  util::StatusOr<uint32> RenderList(const google::protobuf::Field* field,
                                      StringPiece name, uint32 list_tag,
                                      ObjectWriter* ow) const;
  // Renders a NWP map.
  // Returns the next tag after reading all map entries. The caller should use
  // this tag before reading more tags from the stream.
  util::StatusOr<uint32> RenderMap(const google::protobuf::Field* field,
                                     StringPiece name, uint32 list_tag,
                                     ObjectWriter* ow) const;

  // Renders a packed repeating field. A packed field is stored as:
  // {tag length item1 item2 item3} instead of the less efficient
  // {tag item1 tag item2 tag item3}.
  util::Status RenderPacked(const google::protobuf::Field* field,
                              ObjectWriter* ow) const;

  // Renders a google.protobuf.Timestamp value to ObjectWriter
  static util::Status RenderTimestamp(const ProtoStreamObjectSource* os,
                                        const google::protobuf::Type& type,
                                        StringPiece name, ObjectWriter* ow);

  // Renders a google.protobuf.Duration value to ObjectWriter
  static util::Status RenderDuration(const ProtoStreamObjectSource* os,
                                       const google::protobuf::Type& type,
                                       StringPiece name, ObjectWriter* ow);

  // Following RenderTYPE functions render well known types in
  // google/protobuf/wrappers.proto corresponding to TYPE.
  static util::Status RenderDouble(const ProtoStreamObjectSource* os,
                                     const google::protobuf::Type& type,
                                     StringPiece name, ObjectWriter* ow);
  static util::Status RenderFloat(const ProtoStreamObjectSource* os,
                                    const google::protobuf::Type& type,
                                    StringPiece name, ObjectWriter* ow);
  static util::Status RenderInt64(const ProtoStreamObjectSource* os,
                                    const google::protobuf::Type& type,
                                    StringPiece name, ObjectWriter* ow);
  static util::Status RenderUInt64(const ProtoStreamObjectSource* os,
                                     const google::protobuf::Type& type,
                                     StringPiece name, ObjectWriter* ow);
  static util::Status RenderInt32(const ProtoStreamObjectSource* os,
                                    const google::protobuf::Type& type,
                                    StringPiece name, ObjectWriter* ow);
  static util::Status RenderUInt32(const ProtoStreamObjectSource* os,
                                     const google::protobuf::Type& type,
                                     StringPiece name, ObjectWriter* ow);
  static util::Status RenderBool(const ProtoStreamObjectSource* os,
                                   const google::protobuf::Type& type,
                                   StringPiece name, ObjectWriter* ow);
  static util::Status RenderString(const ProtoStreamObjectSource* os,
                                     const google::protobuf::Type& type,
                                     StringPiece name, ObjectWriter* ow);
  static util::Status RenderBytes(const ProtoStreamObjectSource* os,
                                    const google::protobuf::Type& type,
                                    StringPiece name, ObjectWriter* ow);

  // Renders a google.protobuf.Struct to ObjectWriter.
  static util::Status RenderStruct(const ProtoStreamObjectSource* os,
                                     const google::protobuf::Type& type,
                                     StringPiece name, ObjectWriter* ow);

  // Helper to render google.protobuf.Struct's Value fields to ObjectWriter.
  static util::Status RenderStructValue(const ProtoStreamObjectSource* os,
                                          const google::protobuf::Type& type,
                                          StringPiece name, ObjectWriter* ow);

  // Helper to render google.protobuf.Struct's ListValue fields to ObjectWriter.
  static util::Status RenderStructListValue(
      const ProtoStreamObjectSource* os, const google::protobuf::Type& type,
      StringPiece name, ObjectWriter* ow);

  // Render the "Any" type.
  static util::Status RenderAny(const ProtoStreamObjectSource* os,
                                  const google::protobuf::Type& type,
                                  StringPiece name, ObjectWriter* ow);

  // Render the "FieldMask" type.
  static util::Status RenderFieldMask(const ProtoStreamObjectSource* os,
                                        const google::protobuf::Type& type,
                                        StringPiece name, ObjectWriter* ow);

  static hash_map<string, TypeRenderer>* renderers_;
  static void InitRendererMap();
  static void DeleteRendererMap();
  static TypeRenderer* FindTypeRenderer(const string& type_url);

  // Renders a field value to the ObjectWriter.
  util::Status RenderField(const google::protobuf::Field* field,
                             StringPiece field_name, ObjectWriter* ow) const;

  // Same as above but renders all non-message field types. Callers don't call
  // this function directly. They just use RenderField.
  util::Status RenderNonMessageField(const google::protobuf::Field* field,
                                       StringPiece field_name,
                                       ObjectWriter* ow) const;


  // Reads field value according to Field spec in 'field' and returns the read
  // value as string. This only works for primitive datatypes (no message
  // types).
  const string ReadFieldValueAsString(
      const google::protobuf::Field& field) const;

  // Utility function to detect proto maps. The 'field' MUST be repeated.
  bool IsMap(const google::protobuf::Field& field) const;

  // Utility to read int64 and int32 values from a message type in stream_.
  // Used for reading google.protobuf.Timestamp and Duration messages.
  std::pair<int64, int32> ReadSecondsAndNanos(
      const google::protobuf::Type& type) const;

  // Helper function to check recursion depth and increment it. It will return
  // Status::OK if the current depth is allowed. Otherwise an error is returned.
  // type_name and field_name are used for error reporting.
  util::Status IncrementRecursionDepth(StringPiece type_name,
                                         StringPiece field_name) const;

  // Input stream to read from. Ownership rests with the caller.
  google::protobuf::io::CodedInputStream* stream_;

  // Type information for all the types used in the descriptor. Used to find
  // google::protobuf::Type of nested messages/enums.
  const TypeInfo* typeinfo_;
  // Whether this class owns the typeinfo_ object. If true the typeinfo_ object
  // should be deleted in the destructor.
  bool own_typeinfo_;

  // google::protobuf::Type of the message source.
  const google::protobuf::Type& type_;


  // Whether to render enums using lowerCamelCase. Defaults to false.
  bool use_lower_camel_for_enums_;

  // Tracks current recursion depth.
  mutable int recursion_depth_;

  // Maximum allowed recursion depth.
  int max_recursion_depth_;

  // Whether to render unknown fields.
  bool render_unknown_fields_;

  GOOGLE_DISALLOW_IMPLICIT_CONSTRUCTORS(ProtoStreamObjectSource);
};

}  // namespace converter
}  // namespace util
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_UTIL_CONVERTER_PROTOSTREAM_OBJECTSOURCE_H__
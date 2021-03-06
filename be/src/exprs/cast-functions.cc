// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "exprs/cast-functions.h"

#include <boost/lexical_cast.hpp>
#include "exprs/anyval-util.h"
#include "exprs/decimal-functions.h"
#include "runtime/timestamp-value.h"
#include "util/string-parser.h"
#include "string-functions.h"

using namespace boost;
using namespace impala;
using namespace impala_udf;
using namespace std;

#define CAST_FUNCTION(from_type, to_type) \
  to_type CastFunctions::CastTo##to_type(FunctionContext* ctx, const from_type& val) { \
    if (val.is_null) return to_type::null(); \
    return to_type(val.val); \
  }

CAST_FUNCTION(TinyIntVal, BooleanVal)
CAST_FUNCTION(SmallIntVal, BooleanVal)
CAST_FUNCTION(IntVal, BooleanVal)
CAST_FUNCTION(BigIntVal, BooleanVal)
CAST_FUNCTION(FloatVal, BooleanVal)
CAST_FUNCTION(DoubleVal, BooleanVal)

CAST_FUNCTION(BooleanVal, TinyIntVal)
CAST_FUNCTION(SmallIntVal, TinyIntVal)
CAST_FUNCTION(IntVal, TinyIntVal)
CAST_FUNCTION(BigIntVal, TinyIntVal)
CAST_FUNCTION(FloatVal, TinyIntVal)
CAST_FUNCTION(DoubleVal, TinyIntVal)

CAST_FUNCTION(BooleanVal, SmallIntVal)
CAST_FUNCTION(TinyIntVal, SmallIntVal)
CAST_FUNCTION(IntVal, SmallIntVal)
CAST_FUNCTION(BigIntVal, SmallIntVal)
CAST_FUNCTION(FloatVal, SmallIntVal)
CAST_FUNCTION(DoubleVal, SmallIntVal)

CAST_FUNCTION(BooleanVal, IntVal)
CAST_FUNCTION(TinyIntVal, IntVal)
CAST_FUNCTION(SmallIntVal, IntVal)
CAST_FUNCTION(BigIntVal, IntVal)
CAST_FUNCTION(FloatVal, IntVal)
CAST_FUNCTION(DoubleVal, IntVal)

CAST_FUNCTION(BooleanVal, BigIntVal)
CAST_FUNCTION(TinyIntVal, BigIntVal)
CAST_FUNCTION(SmallIntVal, BigIntVal)
CAST_FUNCTION(IntVal, BigIntVal)
CAST_FUNCTION(FloatVal, BigIntVal)
CAST_FUNCTION(DoubleVal, BigIntVal)

CAST_FUNCTION(BooleanVal, FloatVal)
CAST_FUNCTION(TinyIntVal, FloatVal)
CAST_FUNCTION(SmallIntVal, FloatVal)
CAST_FUNCTION(IntVal, FloatVal)
CAST_FUNCTION(BigIntVal, FloatVal)
CAST_FUNCTION(DoubleVal, FloatVal)

CAST_FUNCTION(BooleanVal, DoubleVal)
CAST_FUNCTION(TinyIntVal, DoubleVal)
CAST_FUNCTION(SmallIntVal, DoubleVal)
CAST_FUNCTION(IntVal, DoubleVal)
CAST_FUNCTION(BigIntVal, DoubleVal)
CAST_FUNCTION(FloatVal, DoubleVal)

#define CAST_FROM_STRING(num_type, native_type, string_parser_fn) \
  num_type CastFunctions::CastTo##num_type(FunctionContext* ctx, const StringVal& val) { \
    if (val.is_null) return num_type::null(); \
    StringParser::ParseResult result; \
    num_type ret; \
    ret.val = StringParser::string_parser_fn<native_type>( \
        reinterpret_cast<char*>(val.ptr), val.len, &result); \
    if (UNLIKELY(result != StringParser::PARSE_SUCCESS)) return num_type::null(); \
    return ret; \
  }

CAST_FROM_STRING(TinyIntVal, int8_t, StringToInt)
CAST_FROM_STRING(SmallIntVal, int16_t, StringToInt)
CAST_FROM_STRING(IntVal, int32_t, StringToInt)
CAST_FROM_STRING(BigIntVal, int64_t, StringToInt)
CAST_FROM_STRING(FloatVal, float, StringToFloat)
CAST_FROM_STRING(DoubleVal, double, StringToFloat)

#define CAST_TO_STRING(num_type) \
  StringVal CastFunctions::CastToStringVal(FunctionContext* ctx, const num_type& val) { \
    if (val.is_null) return StringVal::null(); \
    ColumnType rtype = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType()); \
    StringVal sv = AnyValUtil::FromString(ctx, lexical_cast<string>(val.val)); \
    AnyValUtil::TruncateIfNecessary(rtype, &sv); \
    return sv; \
  }

CAST_TO_STRING(BooleanVal);
CAST_TO_STRING(SmallIntVal);
CAST_TO_STRING(IntVal);
CAST_TO_STRING(BigIntVal);

// Special case for float types to string that deals properly with nan
// (lexical_cast<string>(nan) returns "-nan" which is nonsensical).
#define CAST_FLOAT_TO_STRING(float_type) \
  StringVal CastFunctions::CastToStringVal(FunctionContext* ctx, const float_type& val) { \
    if (val.is_null) return StringVal::null(); \
    if (isnan(val.val)) return StringVal("nan"); \
    ColumnType rtype = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType()); \
    StringVal sv = AnyValUtil::FromString(ctx, lexical_cast<string>(val.val)); \
    AnyValUtil::TruncateIfNecessary(rtype, &sv); \
    return sv; \
  }

CAST_FLOAT_TO_STRING(FloatVal);
CAST_FLOAT_TO_STRING(DoubleVal);

// Special-case tinyint because boost thinks it's a char and handles it differently.
// e.g. '0' is written as an empty string.
StringVal CastFunctions::CastToStringVal(FunctionContext* ctx, const TinyIntVal& val) {
  if (val.is_null) return StringVal::null();
  int64_t tmp_val = val.val;
  ColumnType rtype = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType());
  StringVal sv = AnyValUtil::FromString(ctx, lexical_cast<string>(tmp_val));
  AnyValUtil::TruncateIfNecessary(rtype, &sv);
  return sv;
}

StringVal CastFunctions::CastToStringVal(FunctionContext* ctx, const TimestampVal& val) {
  if (val.is_null) return StringVal::null();
  TimestampValue tv = TimestampValue::FromTimestampVal(val);
  ColumnType rtype = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType());
  StringVal sv = AnyValUtil::FromString(ctx, lexical_cast<string>(tv));
  AnyValUtil::TruncateIfNecessary(rtype, &sv);
  return sv;
}

StringVal CastFunctions::CastToStringVal(FunctionContext* ctx, const StringVal& val) {
  if (val.is_null) return StringVal::null();
  StringVal sv;
  ColumnType type = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType());
  sv.ptr = val.ptr;
  sv.len = val.len;
  AnyValUtil::TruncateIfNecessary(type, &sv);
  return sv;
}

StringVal CastFunctions::CastToChar(FunctionContext* ctx, const StringVal& val) {
  if (val.is_null) return StringVal::null();

  ColumnType type = AnyValUtil::TypeDescToColumnType(ctx->GetReturnType());
  DCHECK(type.type == TYPE_CHAR);
  DCHECK_GE(type.len, 1);
  char* cptr;
  if (type.len > val.len) {
    cptr = reinterpret_cast<char*>(ctx->impl()->AllocateLocal(type.len));
    memcpy(cptr, val.ptr, min(type.len, val.len));
    StringValue::PadWithSpaces(cptr, type.len, val.len);
  } else {
    cptr = reinterpret_cast<char*>(val.ptr);
  }
  StringVal sv;
  sv.ptr = reinterpret_cast<uint8_t*>(cptr);
  sv.len = type.len;
  return sv;
}

#define CAST_FROM_TIMESTAMP(to_type) \
  to_type CastFunctions::CastTo##to_type( \
      FunctionContext* ctx, const TimestampVal& val) { \
    if (val.is_null) return to_type::null(); \
    TimestampValue tv = TimestampValue::FromTimestampVal(val); \
    return to_type(tv); \
  }

CAST_FROM_TIMESTAMP(BooleanVal);
CAST_FROM_TIMESTAMP(TinyIntVal);
CAST_FROM_TIMESTAMP(SmallIntVal);
CAST_FROM_TIMESTAMP(IntVal);
CAST_FROM_TIMESTAMP(BigIntVal);
CAST_FROM_TIMESTAMP(FloatVal);
CAST_FROM_TIMESTAMP(DoubleVal);

#define CAST_TO_TIMESTAMP(from_type) \
  TimestampVal CastFunctions::CastToTimestampVal(FunctionContext* ctx, \
                                                 const from_type& val) { \
    if (val.is_null) return TimestampVal::null(); \
    TimestampValue timestamp_value(val.val); \
    TimestampVal result; \
    timestamp_value.ToTimestampVal(&result); \
    return result; \
  }

CAST_TO_TIMESTAMP(BooleanVal);
CAST_TO_TIMESTAMP(TinyIntVal);
CAST_TO_TIMESTAMP(SmallIntVal);
CAST_TO_TIMESTAMP(IntVal);
CAST_TO_TIMESTAMP(BigIntVal);
CAST_TO_TIMESTAMP(FloatVal);
CAST_TO_TIMESTAMP(DoubleVal);

TimestampVal CastFunctions::CastToTimestampVal(FunctionContext* ctx,
                                               const StringVal& val) {
  if (val.is_null) return TimestampVal::null();
  TimestampValue timestamp_value(reinterpret_cast<char*>(val.ptr), val.len);
  // Return null if 'val' did not parse
  if (timestamp_value.NotADateTime()) return TimestampVal::null();
  TimestampVal result;
  timestamp_value.ToTimestampVal(&result);
  return result;
}

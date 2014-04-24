
enum var_flags_values{
  F_OP_ADD = 0,
  F_OP_SUB,
  F_OP_MUL,
  F_OP_DIV,
  F_OP_MOD,
  F_OP_BITW,
  F_OP_ATTR,
  F_OP_INDX,
  F_OP_CALL,
  F_OP_EQ,
  F_OP_INEQ,
  F_ARG_LEN,
  F_ARG_RANGE,
  F_ARG_FOR,
  F_ARG_ITER,
  F_ARG_INDX,
  F_ARG_CALL,
  F_ARG_IF,
  F_ARG_WHILE,
  F_FL_RETURNED,
  F_FL_RESULT,
  F_FL_LOCAL,
  F_APP_1L,
  F_APP_UNDSC,
  F_TYPE_INT,
  F_TYPE_FLOAT,
  F_TYPE_STR,
  F_TYPE_LIST,
  F_TYPE_TUPLE,
  F_TYPE_DICT,
  F_TYPE_OBJ,

  F_FLAG_INVALID,
  F_MAX, // assert F_MAX < bitsizeof(var_flags->flags)
};



// Generates Documentation
enum DocsFlagKind {
	DocsFlag_Invalid,
	DocsFlag_Format,
	DocsFlag_SkipHidden,
	DocsFlag_COUNT,
};

enum DocsFlagParamKind {
	DocsFlagParam_None,

	DocsFlagParam_Boolean,
	DocsFlagParam_Integer,
	DocsFlagParam_Float,
	DocsFlagParam_String,

	DocsFlagParam_COUNT,
};

struct DocsFlag {
	DocsFlagKind      kind;
	String            name;
	DocsFlagParamKind param_kind;
};

enum DocsFormat {
	DocsFormat_Invalid,

	DocsFormat_Slate,
	DocsFormat_Markdown,
	DocsFormat_Html,
	
	DocsFormat_COUNT,
};

struct DocsContext {
	gbFile     doc_file;

	DocsFormat doc_format;
	bool       skip_set;
	bool 	   skip_hidden;
};

gb_global DocsContext docs_context = {};

#define CHECK_SKIP_CONTINUE(ident, skip) if(ident->token.string[0] == '_' && skip) { \
									         continue; \
									     } 

#define CHECK_SKIP_RETURN(ident, skip) if(ident->token.string[0] == '_' && skip) { \
									       return; \
									   } 

#define CHECK_SKIP_OUT(ident, skip, out) if(ident->token.string[0] == '_' && skip) { \
										     out = true; \
										 } else { \
											 out = false; \
										 }

#define DOC_PRINT(...) gb_fprintf(&docs_context.doc_file, __VA_ARGS__)


#include "docs_helpers.cpp"

void init_docs_context(void) {
	DocsContext *dc = &docs_context;

	if(dc->doc_format == DocsFormat_Invalid) {
		dc->doc_format = DocsFormat_Slate;
	}

	if(!dc->skip_set) {
		dc->skip_hidden = true;
	}
}

void add_flag(Array<DocsFlag> *docs_flags, DocsFlagKind kind, String name, DocsFlagParamKind param_kind) {
	DocsFlag flag = {kind, name, param_kind};
	array_add(docs_flags, flag);
}

bool parse_docs_flags(Array<String> args) {
	Array<DocsFlag> docs_flags = {};
	array_init(&docs_flags, heap_allocator(), DocsFlag_COUNT);
	add_flag(&docs_flags, DocsFlag_Format, str_lit("format"), DocsFlagParam_String);
	add_flag(&docs_flags, DocsFlag_SkipHidden, str_lit("skip-hidden"), DocsFlagParam_Boolean);

	GB_ASSERT(args.count >= 3);
	Array<String> flag_args = args;
	flag_args.data  += 3;
	flag_args.count -= 3;

	bool set_flags[DocsFlag_COUNT] = {};

	bool bad_flags = false;
	for_array(i, flag_args) {
		String flag = flag_args[i];
		if (flag[0] != '-') {
			gb_printf_err("Invalid flag: %.*s\n", LIT(flag));
			continue;
		}
		String name = substring(flag, 1, flag.len);
		isize end = 0;
		for (; end < name.len; end++) {
			if (name[end] == '=') break;
		}
		name = substring(name, 0, end);
		String param = {};
		if (end < flag.len-1) param = substring(flag, 2+end, flag.len);

		bool found = false;
		for_array(docs_flag_index, docs_flags) {
			DocsFlag df = docs_flags[docs_flag_index];
			if (df.name == name) {
				found = true;
				if (set_flags[df.kind]) {
					gb_printf_err("Previous flag set: '%.*s'\n", LIT(name));
					bad_flags = true;
				} else {
					ExactValue value = {};
					bool ok = false;
					if (df.param_kind == DocsFlagParam_None) {
						if (param.len == 0) {
							ok = true;
						} else {
							gb_printf_err("Flag '%.*s' was not expecting a parameter '%.*s'\n", LIT(name), LIT(param));
							bad_flags = true;
						}
					} else if (param.len == 0) {
						gb_printf_err("Flag missing for '%.*s'\n", LIT(name));
						bad_flags = true;
					} else {
						ok = true;
						switch (df.param_kind) {
						default: ok = false; break;
						case DocsFlagParam_Boolean: {
							if (str_eq_ignore_case(param, str_lit("t")) ||
							    str_eq_ignore_case(param, str_lit("true")) ||
							    param == "1") {
								value = exact_value_bool(true);
							} else if (str_eq_ignore_case(param, str_lit("f")) ||
							           str_eq_ignore_case(param, str_lit("false")) ||
							           param == "0") {
								value = exact_value_bool(false);
							} else {
								gb_printf_err("Invalid flag parameter for '%.*s' = '%.*s'\n", LIT(name), LIT(param));
							}
						} break;
						case DocsFlagParam_Integer:
							value = exact_value_integer_from_string(param);
							break;
						case DocsFlagParam_Float:
							value = exact_value_float_from_string(param);
							break;
						case DocsFlagParam_String:
							value = exact_value_string(param);
							break;
						}
					}
					if (ok) {
						switch (df.param_kind) {
						case DocsFlagParam_None:
							if (value.kind != ExactValue_Invalid) {
								gb_printf_err("%.*s expected no value, got %.*s", LIT(name), LIT(param));
								bad_flags = true;
								ok = false;
							}
							break;
						case DocsFlagParam_Boolean:
							if (value.kind != ExactValue_Bool) {
								gb_printf_err("%.*s expected a boolean, got %.*s", LIT(name), LIT(param));
								bad_flags = true;
								ok = false;
							}
							break;
						case DocsFlagParam_Integer:
							if (value.kind != ExactValue_Integer) {
								gb_printf_err("%.*s expected an integer, got %.*s", LIT(name), LIT(param));
								bad_flags = true;
								ok = false;
							}
							break;
						case DocsFlagParam_Float:
							if (value.kind != ExactValue_Float) {
								gb_printf_err("%.*s expected a floating pointer number, got %.*s", LIT(name), LIT(param));
								bad_flags = true;
								ok = false;
							}
							break;
						case DocsFlagParam_String:
							if (value.kind != ExactValue_String) {
								gb_printf_err("%.*s expected a string, got %.*s", LIT(name), LIT(param));
								bad_flags = true;
								ok = false;
							}
							break;
						}

						if (ok) switch (df.kind) {
							case DocsFlag_Format : {
								GB_ASSERT(value.kind == ExactValue_String);
								if(value.value_string == "slate") {
									docs_context.doc_format = DocsFormat_Slate;
								} else if(value.value_string == "html") {
									//docs_context.doc_format = DocsFormat_Html;
									docs_context.doc_format = DocsFormat_Slate;
									gb_printf_err("'Html' format is not implemented, using 'Slate'\n");
								} else if(value.value_string == "markdown") {
									//docs_context.doc_format = DocsFormat_Markdown;
									docs_context.doc_format = DocsFormat_Slate;
									gb_printf_err("'Markdown' format is not implemented, using 'Slate'\n");
								} else {
									gb_printf_err("Unsupported documentation format '%.*s'\n", LIT(value.value_string));
									bad_flags = true;
								}
								break;
							}

							case DocsFlag_SkipHidden : {
								GB_ASSERT(value.kind == ExactValue_Bool);
								docs_context.skip_set    = true;
								docs_context.skip_hidden = value.value_bool;
								break;
							}
						}
					}

					set_flags[df.kind] = ok;
				}
				break;
			}
		}
		if (!found) {
			gb_printf_err("Unknown flag: '%.*s'\n", LIT(name));
			bad_flags = true;
		}
	}

	return !bad_flags;
}

String alloc_comment_group_string(gbAllocator a, CommentGroup g) {
	isize len = 0;
	for_array(i, g.list) {
		String comment = g.list[i].string;
		len += comment.len;
		len += 1; // for \n
	}
	if (len == 0) {
		return make_string(nullptr, 0);
	}

	Array<u8> backing = {};
	array_init(&backing, heap_allocator(), len+1);

	for_array(i, g.list) {
		String comment = g.list[i].string;
		if (comment[1] == '/') {
			comment.text += 2;
			comment.len  -= 2;
		} else if (comment[1] == '*') {
			comment.text += 2;
			comment.len  -= 4;
		}
		comment = string_trim_whitespace(comment);
		for (int i = 0; i < comment.len; ++i)
		{
			array_add(&backing, comment.text[i]);
		}
	}

	return {backing.data, backing.count};
}

void print_anchor(AstNode *node) {
	String result;
	switch (node->kind) {
		case_ast_node(ident, Ident, node);
			String name = ident->token.string;
			Array<u8> backing;
			array_init(&backing, heap_allocator(), name.len);
			while (name.len > 0) {
				Rune rune;
				isize width = gb_utf8_decode(name.text, name.len, &rune);

				name.len -= width;
				name.text += width;
				Rune lower = rune_to_lower(rune);
				u8 str[4] = {};
				isize len = gb_utf8_encode_rune(str, lower);
				for (int i = 0; i < len; ++i)
				{
					array_add(&backing, str[i]);
				}
			}
			String new_name = {backing.data, backing .count};
			DOC_PRINT("#%.*s", LIT(new_name));
		case_end;

		case_ast_node(expr, SelectorExpr, node);
			print_anchor(expr->selector);
		case_end;

		case_ast_node(expr, UnaryExpr, node);
			print_anchor(expr->expr);
		case_end;

		case_ast_node(expr, ParenExpr, node);
			print_anchor(expr->expr);
		case_end;

		case_ast_node(ptr, PointerType, node);
			print_anchor(ptr->type);
		case_end;
		
		case_ast_node(da, DynamicArrayType, node);
			print_anchor(da->elem);
		case_end;
		//NOTE(Hoej): Doesn't print size if specified
		case_ast_node(at, ArrayType, node);
			print_anchor(at->elem);
		case_end;
		
		case_ast_node(lit, CompoundLit, node);
			print_anchor(lit->type);
		case_end;
	} 
}

void print_type(AstNode *node, char *new_line = "\n") {
	String result;
	switch (node->kind) {
		case_ast_node(ident, Ident, node);
			DOC_PRINT("%.*s", LIT(ident->token.string));
		case_end;

		case_ast_node(expr, SelectorExpr, node);
			ast_node(import, Ident, expr->expr);
			ast_node(ident, Ident, expr->selector);
			DOC_PRINT("%.*s.", LIT(import->token.string));
			DOC_PRINT("%.*s", LIT(ident->token.string));
		case_end;

		case_ast_node(expr, UnaryExpr, node);
			DOC_PRINT("%.*s", LIT(expr->op.string));
			print_type(expr->expr);
		case_end;

		case_ast_node(expr, ParenExpr, node);
			DOC_PRINT("(");
			print_type(expr->expr);
			DOC_PRINT(")");
		case_end;

		case_ast_node(expr, BinaryExpr, node);
			print_type(expr->left);
			if (expr->op.string[0] == '|') {
				DOC_PRINT(" \\ ", LIT(expr->op.string));
			} else {
				DOC_PRINT(" %.*s ", LIT(expr->op.string));
			}
			print_type(expr->right);
		case_end;

		case_ast_node(ptr, PointerType, node);
			DOC_PRINT("^");
			print_type(ptr->type);
		case_end;
		
		case_ast_node(da, DynamicArrayType, node);
			DOC_PRINT("[dynamic]");
			print_type(da->elem);
		case_end;
		//NOTE(Hoej): Doesn't print size if specified
		case_ast_node(at, ArrayType, node);
			DOC_PRINT("[");
			if (at->count != nullptr) {
				print_type(at->count);
			}
			DOC_PRINT("]");
			print_type(at->elem);
		case_end;
		
		case_ast_node(lit, BasicLit, node);
			switch (lit->token.kind) {
				case Token_String : 
					DOC_PRINT("\"%.*s\"", LIT(lit->token.string));
					break;

				default :
					DOC_PRINT("%.*s", LIT(lit->token.string));
					break;
			}
		case_end;

		case_ast_node(lit, CompoundLit, node);
			print_type(lit->type);
			DOC_PRINT("{}");
		case_end;

		case_ast_node(el, Ellipsis, node);
			DOC_PRINT("...");
			print_type(el->expr);
		case_end;

		case_ast_node(t, TypeType, node);
			DOC_PRINT("type");
			if(t->specialization != nullptr) {
				print_type(t->specialization);
			}
		case_end;

		case_ast_node(pt, PolyType, node);
			DOC_PRINT("$");
			print_type(pt->type);
			if(pt->specialization != nullptr) {
				DOC_PRINT("/");
				print_type(pt->specialization);
			}
		case_end;

		case_ast_node(m, MapType, node);
			DOC_PRINT("[");
			print_type(m->key);
			DOC_PRINT("]");
			print_type(m->value);
		case_end;

		case_ast_node(bd, BasicDirective, node);
			DOC_PRINT("#");
			DOC_PRINT("%.*s", LIT(bd->name));
		case_end;

		case_ast_node(prt, ProcType, node)
			DOC_PRINT("proc (");
			if(prt->params != nullptr) {
				ast_node(list, FieldList, prt->params);
				for_array(param_index, list->list) {
					AstNode *param =  list->list[param_index];
					print_type(param);
				}
			}

			if(prt->results != nullptr) {
				ast_node(list, FieldList, prt->results);
				DOC_PRINT(") -> (");
				for_array(result_index, list->list) {
					AstNode *result =  list->list[result_index];
					print_type(result);
				}
			}
			DOC_PRINT(")");
		case_end;

		case_ast_node(f, Field, node);
			for_array(names_index, f->names) {
				AstNode *name = f->names[names_index];
				ast_node(ident, Ident, name);
				DOC_PRINT("%.*s", LIT(ident->token.string));
			}

			if (f->type != nullptr) {
				DOC_PRINT(" : ");
				print_type(f->type);
			} else {
				DOC_PRINT(" := ");
				print_type(f->default_value);
			}
		case_end;

		case_ast_node(ut, UnionType, node);
			DOC_PRINT("union { ");
			if(ut->variants.count > 3) {
				DOC_PRINT(new_line);
			}
			for_array(ui, ut->variants) {
				print_type(ut->variants[ui]);
				if(ui != ut->variants.count-1) {
					DOC_PRINT(", ");
				}
				if(ui != ut->variants.count-1) {
					if(ut->variants.count > 3) {
						DOC_PRINT(new_line);
					}
				}
			}
			if(ut->variants.count > 3) {
				DOC_PRINT(new_line);
			}
			DOC_PRINT(" }");
		case_end;

		case_ast_node(ce, CallExpr, node);
			print_type(ce->proc);
			DOC_PRINT("(");
			for_array(arg_index, ce->args) {
				AstNode *arg = ce->args[arg_index];
				print_type(arg);
				if(arg_index != ce->args.count-1) {
					DOC_PRINT(", ");	
				}
			}
			DOC_PRINT(")");
		case_end;

		case_ast_node(imp, Implicit, node);
			DOC_PRINT("%.*s", LIT(imp->string));
		case_end;

		default : 
			DOC_PRINT("???[%.*s]", LIT(ast_node_strings[node->kind]));
			break;
	} 
}

void print_anchor_type(AstNode* type) {
	DOC_PRINT("<a style=\"color:inherit; text-decoration:inherit\" href=\"");
	print_anchor(type);
	DOC_PRINT("\">");
	print_type(type);
	DOC_PRINT("</a>");
}

void print_struct_code(AstNodeIdent *ident, AstNodeStructType *type, bool skip_hidden) {
	CHECK_SKIP_RETURN(ident, skip_hidden);

	DOC_PRINT("%.*s :: struct ", LIT(ident->token.string));
	DOC_PRINT("%s", type->is_packed ? "#packed " : "");
	DOC_PRINT("%s", type->is_raw_union ? "#raw_union " : "");
	DOC_PRINT("{\n");

	for_array(fi, type->fields) {
		ast_node(field, Field, type->fields[fi]);
		bool skip = true;
		for_array(fj, field->names) {
			AstNode *field_name = field->names[fj];
			ast_node(field_ident, Ident, field_name);
			
			CHECK_SKIP_OUT(field_ident, skip_hidden, skip);
			if(skip) {
				continue;
			}

			if(fj == 0) {
				DOC_PRINT("    ");
			}

			DOC_PRINT("%.*s", LIT(field_ident->token.string));
			
			if(fj != field->names.count-1) {
				DOC_PRINT(", ");
			}
		}
		
		if(skip) {
			continue;
		}

		if (field->type != nullptr) {
			DOC_PRINT(" : ");
			print_type(field->type, "");
		} else {
			DOC_PRINT(" := ");
			print_type(field->default_value);
		}

		if(fi != type->fields.count-1) {
			DOC_PRINT(",");
		}

		DOC_PRINT("\n");
	}
	DOC_PRINT("}\n");
}

void print_struct_type(AstNode *name, AstNode *type, CommentGroup docs, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_StructType);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);
	ast_node(st, StructType, type);
	h2(ident);

	//Code
	DOC_PRINT("```go\n");
	if (st->fields.count > 0) {
		print_struct_code(ident, st, skip_hidden);
	} else {
		DOC_PRINT("%.*s :: struct {}\n", LIT(ident->token.string));
	}
	DOC_PRINT("```\n\n");

	bold("Type:");
	DOC_PRINT(" Struct\n\n");
	doc_location(ident);
	DOC_PRINT("\n\n");
/*	if (st->fields.count <= 0) {
		DOC_PRINT("<aside class=\"notice\">This is an opaque struct</aside>\n\n");
	} */

	//Markdown
	if (st->fields.count > 0) {
		DOC_PRINT("\n");
		DOC_PRINT("| Name | Type | |\n");
		DOC_PRINT("|-|-|-|\n");
		bool last_had_desc = false; 
		for_array(fi, st->fields) {
			if(last_had_desc) {
				DOC_PRINT("| | | |\n");
				DOC_PRINT("|-|-|-|\n");
				last_had_desc = false;
			}
			ast_node(field, Field, st->fields[fi]);
			//Print names
			bool started = false;
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				CHECK_SKIP_CONTINUE(field_ident, skip_hidden);

				if(!started) {
					DOC_PRINT("| ");
					started = true;
				}

				bold("%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					DOC_PRINT(", ");
				}
			}
			if(!started) {
				continue;
			}

			DOC_PRINT(" | ");

			//Print type
			if (field->type != nullptr) {
				print_anchor_type(field->type);
			} else {
				DOC_PRINT(":= ");
				print_type(field->default_value);
			}
			DOC_PRINT(" | ");

			if(field->comment.list.count > 0) {
				String doc_str = alloc_comment_group_string(heap_allocator(), field->comment);
				DOC_PRINT("%.*s |", LIT(doc_str));
			} else {
				DOC_PRINT(" | ");
			}

			if(field->docs.list.count > 0) {
				DOC_PRINT("\n");
				String doc_str = alloc_comment_group_string(heap_allocator(), field->docs);
				DOC_PRINT("**_Description:_**\n\n%.*s\n\n", LIT(doc_str));
				last_had_desc = true;
			}
			DOC_PRINT("\n");
		}
		DOC_PRINT("\n\n");
	}
}

void print_proc_group(AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_ProcGroup);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);
	ast_node(pg, ProcGroup, type);

	h2(ident);
	bold("Type:");
	DOC_PRINT(" Procedure Group\n\n");
	doc_location(ident);
	DOC_PRINT("\n\n");

	DOC_PRINT("| |\n");
	DOC_PRINT("|-|\n");
	for_array(arg_index, pg->args) {
		ast_node(ident, Ident, pg->args[arg_index]);
		if(ident->token.string[0] == '_') {
			continue;
		}
		DOC_PRINT("|");
		print_anchor_type(pg->args[arg_index]);
		DOC_PRINT("|\n");
	}
}

void print_proc_code(AstNodeIdent *ident, AstNodeProcType *type, bool helper, bool skip_hidden) {
	CHECK_SKIP_RETURN(ident, skip_hidden);
	DOC_PRINT("%.*s :: %sproc(", 
	          LIT(ident->token.string), 
	          helper ? "#type " : "");

	ast_node(list, FieldList, type->params);
	if (list->list.count > 0) {
		DOC_PRINT("\n");
		for_array(list_index, list->list) {
			ast_node(field, Field, list->list[list_index]);
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				if(fj == 0) {
					DOC_PRINT("    ");
				}
				DOC_PRINT("%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					DOC_PRINT(", ");
				}
			}

			if (field->type != nullptr) {
				DOC_PRINT(" : ");
				print_type(field->type);
			} else {
				DOC_PRINT(" := ");
				print_type(field->default_value);
			}
			if(list_index != list->list.count-1) {
				DOC_PRINT(", \n");
			}
		}
		DOC_PRINT("\n");
	}
	DOC_PRINT(") ");

	if (type->results != nullptr) {
		DOC_PRINT("-> (");
		ast_node(results, FieldList, type->results);
		for_array(list_index, results->list) {
			ast_node(field, Field, results->list[list_index]);
				for_array(fj, field->names) {
					AstNode *field_name = field->names[fj];
					ast_node(field_ident, Ident, field_name);
					bool skip = false;
					CHECK_SKIP_OUT(field_ident, skip_hidden, skip);
					if(skip) {
						continue;
					}

					DOC_PRINT("%.*s : ", LIT(field_ident->token.string));
				}
				if (field->type != nullptr) {
					print_type(field->type);
					if (list_index != results->list.count-1) {
						DOC_PRINT(", ");
					}
				}
		}
		DOC_PRINT(")\n");
	} 	
}

void print_proc_type(AstNode *name, AstNode *type, bool helper, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_ProcType);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);

	ast_node(pt, ProcType, type);
	h2(ident);

	bold("Type:");
	DOC_PRINT(" Procedure ");
	if(helper) {
		DOC_PRINT("Type");
	}
	DOC_PRINT("\n\n");
	
	doc_location(ident);
	DOC_PRINT("\n\n");
	

	//Code
	DOC_PRINT("```go\n");
	print_proc_code(ident, pt, helper, skip_hidden);
	DOC_PRINT("\n```\n\n");

	//Markdown
	ast_node(list, FieldList, pt->params);
	if (list->list.count > 0) {
		h3("Parameters;");
		DOC_PRINT("| Parameter | Type |\n");
		DOC_PRINT("|-|-|\n");
		for_array(list_index, list->list) {
			DOC_PRINT("| ");
			ast_node(field, Field, list->list[list_index]);
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				DOC_PRINT("%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					DOC_PRINT(", ");
				}
			}

			if (field->type != nullptr) {
				DOC_PRINT(" | ");
				print_anchor_type(field->type);
			} else {
				DOC_PRINT("| := ");
				print_type(field->default_value);
			}
			DOC_PRINT(" |");
			DOC_PRINT("\n");
		}
	}
	DOC_PRINT("\n\n");

	if (pt->results != nullptr) {
		h3("Returns;");
		DOC_PRINT("| Name | Type |\n");
		DOC_PRINT("|-|-|\n");
		ast_node(results, FieldList, pt->results);
		for_array(list_index, results->list) {
			DOC_PRINT("| ");
			ast_node(field, Field, results->list[list_index]);
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				DOC_PRINT("%.*s ", LIT(field_ident->token.string));
			}
			
			DOC_PRINT("| ");

			if (field->type != nullptr) {
				DOC_PRINT(" | ");
				print_anchor_type(field->type);
			}
			DOC_PRINT(" |\n");
		}
	} 	

	DOC_PRINT("\n\n");

}

void print_enum_type(AstNode *name, AstNode *type, CommentGroup docs, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_EnumType);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);
	ast_node(et, EnumType, type);
	
	h2(ident);
	bold("Type:");
	DOC_PRINT(" Enum\n\n");
	doc_location(ident);
	DOC_PRINT("\n\n");

	DOC_PRINT("| Name | Value | Desc |\n");
	DOC_PRINT("|-|-|-|\n");
	for_array(fi, et->fields) {
		DOC_PRINT("| ");
		AstNode *f = et->fields[fi];
		switch (f->kind) {
			case AstNode_Ident :
				print_type(f);
				DOC_PRINT(" | ");
				//NOTE(Hoej): Value not printed in this case
				DOC_PRINT(" | ");
				//TODO(Hoej): Print docs when added
				DOC_PRINT(" |");
				DOC_PRINT("\n");
				break;

			case_ast_node(fv, FieldValue, f)
				print_type(fv->field);
				DOC_PRINT(" | ");
				print_anchor_type(fv->value);
				DOC_PRINT(" | ");
				//TODO(Hoej): Print docs when added
				DOC_PRINT(" |");
				DOC_PRINT("\n");
			case_end;
		}

	}
	DOC_PRINT("\n\n");
}

void print_const_type(AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);
	//String docstr = alloc_comment_group_string(heap_allocator(), docs);
	//DOC_PRINT("(%.*s:%d)\n", LIT(ident->token.pos.file), ident->token.pos.line);
	//DOC_PRINT("%.*s", LIT(docstr));
	DOC_PRINT("`%.*s :: ", LIT(ident->token.string));
	print_type(type);
	DOC_PRINT("`\n\n");
}

void print_union_type(AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_UnionType);
	ast_node(ident, Ident, name);
	CHECK_SKIP_RETURN(ident, skip_hidden);
	ast_node(ut, UnionType, type);

	h2(ident);

	DOC_PRINT("```go\n");
	DOC_PRINT("%.*s :: union {\n", LIT(ident->token.string));
	for_array(var_index, ut->variants) {
		AstNode *variant = ut->variants[var_index];
		DOC_PRINT("    ");
		print_type(variant);
		if(var_index != ut->variants.count-1) {
			DOC_PRINT(", ");
		}
		DOC_PRINT("\n");
	}
	DOC_PRINT("}\n```\n");

	//Markdown
	bold("Type:");
	DOC_PRINT(" Union\n\n");
	doc_location(ident);
	DOC_PRINT("\n\n");
	
	DOC_PRINT("| Variant | Desc |\n");
	DOC_PRINT("|-|-|\n");
	for_array(var_index, ut->variants) {
		DOC_PRINT("|");
		AstNode *variant = ut->variants[var_index];
		print_anchor_type(variant);
		DOC_PRINT("|");
		DOC_PRINT(" |");
		DOC_PRINT("\n");
	}
}

void print_declaration(AstNode *decl) {
	switch (decl->kind) {
		case_ast_node(vd, ValueDecl, decl);
			isize max = gb_min(vd->names.count, vd->values.count);
			for (isize i = 0; i < max; i++) {
				AstNode *name = vd->names[i];
				AstNode *value = vd->values[i];
				switch (value->kind) {
					case AstNode_StructType : 
						print_struct_type(name, value, vd->docs, docs_context.skip_hidden);
						break;
					case AstNode_EnumType :
						print_enum_type(name, value, vd->docs, docs_context.skip_hidden);
						break;
					case_ast_node(lit, ProcLit, value);
						print_proc_type(name, lit->type, false, docs_context.skip_hidden);
					case_end;

					case_ast_node(help, HelperType, value);
						print_proc_type(name, help->type, true, docs_context.skip_hidden); 
					case_end;

					case AstNode_BasicLit :
					case AstNode_BinaryExpr :
					case AstNode_UnaryExpr :
					case AstNode_Ident :
					case AstNode_SelectorExpr :
					case AstNode_DynamicArrayType :
						print_const_type(name, value, docs_context.skip_hidden); 
						break;

					case AstNode_ProcGroup : 
						print_proc_group(name, value, docs_context.skip_hidden);
						break;

					case AstNode_UnionType :
						print_union_type(name, value, docs_context.skip_hidden);
						break;

					default :
						gb_printf("DONT KNOW HOW TO PRINT: %.*s\n", LIT(ast_node_strings[value->kind]));
						break;
				}
			}
		case_end;

		default : 
			gb_printf("DECL NOT HANDLED: %.*s\n", LIT(ast_node_strings[decl->kind]));
			break;
	}
}

void document_file(AstFile *file) {
	GB_ASSERT(file != nullptr);
		
	Tokenizer *tokenizer = &file->tokenizer;
	String fullpath = tokenizer->fullpath;
	char doc_name[1024];
	gb_snprintf(doc_name, 1024, "%.*s.html.md", LIT(filename_from_path(fullpath)));
	gb_file_create(&docs_context.doc_file, doc_name);
	gb_printf("------ Parsing: %.*s ------\n", LIT(fullpath));

	//Collect Decls
	Array<AstNode *> type_decls   = make_ast_node_array(file);
	Array<AstNode *> proc_decls   = make_ast_node_array(file);
	Array<AstNode *> enum_decls   = make_ast_node_array(file);
	Array<AstNode *> helper_decls = make_ast_node_array(file);
	Array<AstNode *> const_decls  = make_ast_node_array(file);


	for_array(decl_index, file->decls) {
		AstNode *decl = file->decls[decl_index];
		switch (decl->kind) {
			case_ast_node(vd, ValueDecl, decl);
				isize max = gb_min(vd->names.count, vd->values.count);
				for (isize i = 0; i < max; i++) {
					ast_node(ident, Ident, vd->names[i]);
					if(ident->token.string[0] == '_') {
						continue;
					}
					AstNode *value = vd->values[i];
					switch (value->kind) {
						case AstNode_UnionType :
						case AstNode_StructType : 
							array_add(&type_decls, decl);
							break;
						case AstNode_ProcGroup :
						case AstNode_ProcLit : 
						case AstNode_HelperType :
							array_add(&proc_decls, decl);
							break;
						case AstNode_EnumType : 
							array_add(&enum_decls, decl);
							break;
						
						//These need to handled in a different way, since some of them might be a typedef
						case AstNode_BasicLit :
						case AstNode_BinaryExpr :
						case AstNode_UnaryExpr :
						case AstNode_Ident : 
						case AstNode_SelectorExpr :
						case AstNode_DynamicArrayType :
							array_add(&const_decls, decl);
							break;

						default : 
							gb_printf("VALUE DECL NOT HANDLED: %.*s\n", LIT(ast_node_strings[value->kind]));
							break;
					}
				}
			case_end;

			case_ast_node(id, ForeignBlockDecl, decl);
				//TODO(Hoej): Not sure how to handle this, seperate category? intermingle with current ones?
			case_end;

			case_ast_node(id, ImportDecl, decl);
				if (id->file == nullptr) {
					break;
				}
				//TODO(Hoej): File stored in ctx now, figure out. 
				//document_file(id->file);
			case_end;

			case_ast_node(ed, ExportDecl, decl);
				if (ed->file == nullptr) {
					break;
				}
				//TODO(Hoej): File stored in ctx now, figure out. 
				//document_file(ed->file);
			case_end;

			case AstNode_ForeignImportDecl : 
				//Intentionally not handled
				break;

			default : 
				gb_printf("DECL NOT HANDLED: %.*s\n", LIT(ast_node_strings[decl->kind]));
			break;
		}
	}

	if(const_decls.count > 0) {
		gb_fprintf(&docs_context.doc_file, "# %.*s - Constants\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, const_decls) {
			AstNode *decl = const_decls[decl_index];
			print_declaration(decl);
		}
	}
	if(type_decls.count > 0) {
		gb_fprintf(&docs_context.doc_file, "# %.*s - Types\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, type_decls) {
			AstNode *decl = type_decls[decl_index];
			print_declaration(decl);
		}
	}
	if(proc_decls.count > 0) {
		gb_fprintf(&docs_context.doc_file, "# %.*s - Procs\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, proc_decls) {
			AstNode *decl = proc_decls[decl_index];
			print_declaration(decl);
		}
	}
	if(enum_decls.count > 0) {
		gb_fprintf(&docs_context.doc_file, "# %.*s - Enums\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, enum_decls) {
			AstNode *decl = enum_decls[decl_index];
			print_declaration(decl);
		}
	}
	gb_printf("------ Done! ------\n\n");
	gb_file_close(&docs_context.doc_file);
}

void generate_documentation(Parser *parser) {
	switch (docs_context.doc_format) {
		case DocsFormat_Slate : {
			gb_printf("Generating documentation using 'Slate' format.\n");
		}
	}

	for_array(file_index, parser->files) {
		AstFile *file = parser->files[file_index];
		document_file(file);
	}
}

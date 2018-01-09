// Generates Documentation
enum DocsFlagKind {
	DocsFlag_Invalid,
	DocsFlag_Format,
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
	DocsFormat doc_format;
};

gb_global DocsContext docs_context = {};

void init_docs_context(void) {
	DocsContext *dc = &docs_context;

	if(dc->doc_format == DocsFormat_Invalid) {
		dc->doc_format = DocsFormat_Slate;
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
			if(comment.text[i] == '\n') {
				//array_add(&backing, cast(u8)'\n');
			}
		}

		//array_add(&backing, cast(u8)'\n'); 
	}

	return {backing.data, backing.count};
}

void print_anchor(gbFile *doc_file, AstNode *node) {
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
			gb_fprintf(doc_file, "#%.*s", LIT(new_name));
		case_end;

		case_ast_node(expr, SelectorExpr, node);
			print_anchor(doc_file, expr->selector);
		case_end;

		case_ast_node(expr, UnaryExpr, node);
			print_anchor(doc_file, expr->expr);
		case_end;

		case_ast_node(expr, ParenExpr, node);
			print_anchor(doc_file, expr->expr);
		case_end;

		case_ast_node(ptr, PointerType, node);
			print_anchor(doc_file, ptr->type);
		case_end;
		
		case_ast_node(da, DynamicArrayType, node);
			print_anchor(doc_file, da->elem);
		case_end;
		//NOTE(Hoej): Doesn't print size if specified
		case_ast_node(at, ArrayType, node);
			print_anchor(doc_file, at->elem);
		case_end;
		
		case_ast_node(lit, CompoundLit, node);
			print_anchor(doc_file, lit->type);
		case_end;
	} 
}

void print_type(gbFile *doc_file, AstNode *node, char *new_line = "\n") {
	String result;
	switch (node->kind) {
		case_ast_node(ident, Ident, node);
			gb_fprintf(doc_file, "%.*s", LIT(ident->token.string));
		case_end;

		case_ast_node(expr, SelectorExpr, node);
			ast_node(import, Ident, expr->expr);
			ast_node(ident, Ident, expr->selector);
			gb_fprintf(doc_file, "%.*s.", LIT(import->token.string));
			gb_fprintf(doc_file, "%.*s", LIT(ident->token.string));
		case_end;

		case_ast_node(expr, UnaryExpr, node);
			gb_fprintf(doc_file, "%.*s", LIT(expr->op.string));
			print_type(doc_file, expr->expr);
		case_end;

		case_ast_node(expr, ParenExpr, node);
			gb_fprintf(doc_file, "(");
			print_type(doc_file, expr->expr);
			gb_fprintf(doc_file, ")");
		case_end;

		case_ast_node(expr, BinaryExpr, node);
			print_type(doc_file, expr->left);
			if (expr->op.string[0] == '|') {
				gb_fprintf(doc_file, " \\ ", LIT(expr->op.string));
			} else {
				gb_fprintf(doc_file, " %.*s ", LIT(expr->op.string));
			}
			print_type(doc_file, expr->right);
		case_end;

		case_ast_node(ptr, PointerType, node);
			gb_fprintf(doc_file, "^");
			print_type(doc_file, ptr->type);
		case_end;
		
		case_ast_node(da, DynamicArrayType, node);
			gb_fprintf(doc_file, "[dynamic]");
			print_type(doc_file, da->elem);
		case_end;
		//NOTE(Hoej): Doesn't print size if specified
		case_ast_node(at, ArrayType, node);
			gb_fprintf(doc_file, "[");
			if (at->count != nullptr) {
				print_type(doc_file, at->count);
			}
			gb_fprintf(doc_file, "]");
			print_type(doc_file, at->elem);
		case_end;
		
		case_ast_node(lit, BasicLit, node);
			switch (lit->token.kind) {
				case Token_String : 
					gb_fprintf(doc_file, "\"%.*s\"", LIT(lit->token.string));
					break;

				default :
					gb_fprintf(doc_file, "%.*s", LIT(lit->token.string));
					break;
			}
		case_end;

		case_ast_node(lit, CompoundLit, node);
			print_type(doc_file, lit->type);
			gb_fprintf(doc_file, "{}");
		case_end;

		case_ast_node(el, Ellipsis, node);
			gb_fprintf(doc_file, "...");
			print_type(doc_file, el->expr);
		case_end;

		case_ast_node(t, TypeType, node);
			gb_fprintf(doc_file, "type");
			if(t->specialization != nullptr) {
				print_type(doc_file, t->specialization);
			}
		case_end;

		case_ast_node(pt, PolyType, node);
			gb_fprintf(doc_file, "$");
			print_type(doc_file, pt->type);
			if(pt->specialization != nullptr) {
				gb_fprintf(doc_file, "/");
				print_type(doc_file, pt->specialization);
			}
		case_end;

		case_ast_node(m, MapType, node);
			gb_fprintf(doc_file, "[");
			print_type(doc_file, m->key);
			gb_fprintf(doc_file, "]");
			print_type(doc_file, m->value);
		case_end;

		case_ast_node(bd, BasicDirective, node);
			gb_fprintf(doc_file, "#");
			gb_fprintf(doc_file, "%.*s", LIT(bd->name));
		case_end;

		case_ast_node(prt, ProcType, node)
			gb_fprintf(doc_file, "proc (");
			if(prt->params != nullptr) {
				ast_node(list, FieldList, prt->params);
				for_array(param_index, list->list) {
					AstNode *param =  list->list[param_index];
					print_type(doc_file, param);
				}
			}

			if(prt->results != nullptr) {
				ast_node(list, FieldList, prt->results);
				gb_fprintf(doc_file, ") -> (");
				for_array(result_index, list->list) {
					AstNode *result =  list->list[result_index];
					print_type(doc_file, result);
				}
			}
			gb_fprintf(doc_file, ")");
		case_end;

		case_ast_node(f, Field, node);
			for_array(names_index, f->names) {
				AstNode *name = f->names[names_index];
				ast_node(ident, Ident, name);
				gb_fprintf(doc_file, "%.*s", LIT(ident->token.string));
			}

			if (f->type != nullptr) {
				gb_fprintf(doc_file, " : ");
				print_type(doc_file, f->type);
			} else {
				gb_fprintf(doc_file, " := ");
				print_type(doc_file, f->default_value);
			}
		case_end;

		case_ast_node(ut, UnionType, node);
			gb_fprintf(doc_file, "union { ");
			if(ut->variants.count > 3) {
				gb_fprintf(doc_file, new_line);
			}
			for_array(ui, ut->variants) {
				print_type(doc_file, ut->variants[ui]);
				if(ui != ut->variants.count-1) {
					gb_fprintf(doc_file, ", ");
				}
				if(ui != ut->variants.count-1) {
					if(ut->variants.count > 3) {
						gb_fprintf(doc_file, new_line);
					}
				}
			}
			if(ut->variants.count > 3) {
				gb_fprintf(doc_file, new_line);
			}
			gb_fprintf(doc_file, " }");
		case_end;

		case_ast_node(ce, CallExpr, node);
			print_type(doc_file, ce->proc);
			gb_fprintf(doc_file, "(");
			for_array(arg_index, ce->args) {
				AstNode *arg = ce->args[arg_index];
				print_type(doc_file, arg);
				if(arg_index != ce->args.count-1) {
					gb_fprintf(doc_file, ", ");	
				}
			}
			gb_fprintf(doc_file, ")");
		case_end;

		case_ast_node(imp, Implicit, node);
			gb_fprintf(doc_file, "%.*s", LIT(imp->string));
		case_end;

		default : 
			gb_fprintf(doc_file, "???[%.*s]", LIT(ast_node_strings[node->kind]));
			break;
	} 
}

void print_struct_type(gbFile *doc_file, AstNode *name, AstNode *type, CommentGroup docs, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_StructType);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}
	ast_node(st, StructType, type);
	gb_fprintf(doc_file, "## %.*s\n\n", LIT(ident->token.string));

	//Code
	gb_fprintf(doc_file, "```go\n");
	if (st->fields.count > 0) {
		
		gb_fprintf(doc_file, "%.*s :: struct ", LIT(ident->token.string));
		gb_fprintf(doc_file, "%s", st->is_packed ? "#packed " : "");
		gb_fprintf(doc_file, "%s", st->is_raw_union ? "#raw_union " : "");
		gb_fprintf(doc_file, "{\n");
		for_array(fi, st->fields) {
			ast_node(field, Field, st->fields[fi]);
			bool skip = true;
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				if(field_ident->token.string[0] == '_' && skip_hidden) {
					continue;
				} else {
					skip = false;
				}
				if(fj == 0) {
					gb_fprintf(doc_file, "    ");
				}
				gb_fprintf(doc_file, "%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					gb_fprintf(doc_file, ", ");
				}
			}
			
			if(skip) {
				continue;
			}

			if (field->type != nullptr) {
				gb_fprintf(doc_file, " : ");
				print_type(doc_file, field->type, "");
			} else {
				gb_fprintf(doc_file, " := ");
				print_type(doc_file, field->default_value);
			}
			if(fi != st->fields.count-1) {
				gb_fprintf(doc_file, ",");
			}
			gb_fprintf(doc_file, "\n");
		}
		gb_fprintf(doc_file, "}\n");
	} else {
		gb_fprintf(doc_file, "%.*s :: struct {}\n", LIT(ident->token.string));
	}
	gb_fprintf(doc_file, "```\n");


	gb_fprintf(doc_file, "Location: %.*s:%d\n\n", LIT(ident->token.pos.file), ident->token.pos.line);
	if (st->fields.count <= 0) {
		gb_fprintf(doc_file, "<aside class=\"notice\">This is an opaque struct</aside>\n\n");
	} 
/*	String doc_str = alloc_comment_group_string(heap_allocator(), docs);
	gb_fprintf(doc_file, "%.*s", LIT(doc_str));*/

	//Markdown
	if (st->fields.count > 0) {
		gb_fprintf(doc_file, "\n");
		gb_fprintf(doc_file, "| Name | Type | |\n");
		gb_fprintf(doc_file, "|-|-|-|\n");
		bool last_had_desc = false; 
		for_array(fi, st->fields) {
			if(last_had_desc) {
				gb_fprintf(doc_file, "| | | |\n");
				gb_fprintf(doc_file, "|-|-|-|\n");
				last_had_desc = false;
			}
			ast_node(field, Field, st->fields[fi]);
			bool skip = true;
			//Print names
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				if(field_ident->token.string[0] == '_' && skip_hidden) {
					continue;
				} else {
					skip = false;
				}

				gb_fprintf(doc_file, "| **%.*s** | ", LIT(field_ident->token.string));
			}
			
			if(skip) {
				continue;
			}
			//Print type
			if (field->type != nullptr) {
				gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
				print_anchor(doc_file, field->type);
				gb_fprintf(doc_file, "\">");
				print_type(doc_file, field->type, "<br/>");
				gb_fprintf(doc_file, "</a>");
			} else {
				gb_fprintf(doc_file, ":= ");
				print_type(doc_file, field->default_value);
			}

			if(field->comment.list.count > 0) {
				String doc_str = alloc_comment_group_string(heap_allocator(), field->comment);
				gb_fprintf(doc_file, " | %.*s |", LIT(doc_str));
				//gb_fprintf(doc_file, "\n");
			}

			if(field->docs.list.count > 0) {
				String doc_str = alloc_comment_group_string(heap_allocator(), field->docs);
				gb_fprintf(doc_file, " \n**_Description:_**\n\n%.*s\n", LIT(doc_str));
				gb_fprintf(doc_file, "\n");
				last_had_desc = true;
			}
			gb_fprintf(doc_file, "\n");
		}
		gb_fprintf(doc_file, "\n\n");
	}
}

void print_proc_group(gbFile *doc_file, AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_ProcGroup);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}
	ast_node(pg, ProcGroup, type);

	gb_fprintf(doc_file, "## %.*s\n\n", LIT(ident->token.string));
	gb_fprintf(doc_file, "Location: %.*s:%d\n\n", LIT(ident->token.pos.file), ident->token.pos.line);

	gb_fprintf(doc_file, "| |\n");
	gb_fprintf(doc_file, "|-|\n");
	for_array(arg_index, pg->args) {
		ast_node(ident, Ident, pg->args[arg_index]);
		if(ident->token.string[0] == '_') {
			continue;
		}
		gb_fprintf(doc_file, "|");
		gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
		print_anchor(doc_file, pg->args[arg_index]);
		gb_fprintf(doc_file, "\">");
		print_type(doc_file, pg->args[arg_index]);
		gb_fprintf(doc_file, "</a>");
		gb_fprintf(doc_file, "|\n");
	}
}

void print_proc_type(gbFile *doc_file, AstNode *name, AstNode *type, bool helper, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_ProcType);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}

	ast_node(pt, ProcType, type);
	gb_fprintf(doc_file, "## %.*s\n\n", LIT(ident->token.string));
	gb_fprintf(doc_file, "Type: Struct\n\n");
	gb_fprintf(doc_file, "Location: %.*s:%d\n\n", LIT(ident->token.pos.file), ident->token.pos.line);

	ast_node(list, FieldList, pt->params);

	//Code
	gb_fprintf(doc_file, "```go\n");
	gb_fprintf(doc_file, "%.*s :: %sproc(", LIT(ident->token.string), helper ? "#type " : "");
	if (list->list.count > 0) {
		gb_fprintf(doc_file, "\n");
		for_array(list_index, list->list) {
			ast_node(field, Field, list->list[list_index]);
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				if(fj == 0) {
					gb_fprintf(doc_file, "    ");
				}
				gb_fprintf(doc_file, "%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					gb_fprintf(doc_file, ", ");
				}
			}

			if (field->type != nullptr) {
				gb_fprintf(doc_file, " : ");
				print_type(doc_file, field->type);
			} else {
				gb_fprintf(doc_file, " := ");
				print_type(doc_file, field->default_value);
			}
			if(list_index != list->list.count-1) {
				gb_fprintf(doc_file, ", \n");
			}
		}
		gb_fprintf(doc_file, "\n");
	}
	gb_fprintf(doc_file, ") ");

	if (pt->results != nullptr) {
		gb_fprintf(doc_file, "-> (");
		ast_node(results, FieldList, pt->results);
		for_array(list_index, results->list) {
			ast_node(field, Field, results->list[list_index]);
				for_array(fj, field->names) {
					AstNode *field_name = field->names[fj];
					ast_node(field_ident, Ident, field_name);
					if(field_ident->token.string[0] == '_') {
						continue;
					}

					gb_fprintf(doc_file, "%.*s : ", LIT(field_ident->token.string));
				}
				if (field->type != nullptr) {
					print_type(doc_file, field->type);
					if (list_index != results->list.count-1) {
						gb_fprintf(doc_file, ", ");
					}
				}
		}
		gb_fprintf(doc_file, ")\n");
	} 	

	gb_fprintf(doc_file, "\n```\n\n");

	//Markdown
	if (list->list.count > 0) {
		gb_fprintf(doc_file, "###Parameters;\n\n");
		gb_fprintf(doc_file, "Parameter | Type\n");
		gb_fprintf(doc_file, "-- | --\n");
		for_array(list_index, list->list) {
			ast_node(field, Field, list->list[list_index]);
			for_array(fj, field->names) {
				AstNode *field_name = field->names[fj];
				ast_node(field_ident, Ident, field_name);
				gb_fprintf(doc_file, "%.*s", LIT(field_ident->token.string));
				if(fj != field->names.count-1) {
					gb_fprintf(doc_file, ", ");
				}
			}

			if (field->type != nullptr) {
				gb_fprintf(doc_file, " | ");
				gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
				print_anchor(doc_file, field->type);
				gb_fprintf(doc_file, "\">");
				print_type(doc_file, field->type);
				gb_fprintf(doc_file, "</a>");
			} else {
				gb_fprintf(doc_file, " | := ");
				print_type(doc_file, field->default_value);
			}
			gb_fprintf(doc_file, "\n");
		}
	}
	gb_fprintf(doc_file, "\n\n");

	if (pt->results != nullptr) {
		gb_fprintf(doc_file, "###Returns;\n\n");
		gb_fprintf(doc_file, "| Name | Type |\n");
		gb_fprintf(doc_file, "|-|-|\n");
		ast_node(results, FieldList, pt->results);
		for_array(list_index, results->list) {
			ast_node(field, Field, results->list[list_index]);
				for_array(fj, field->names) {
					AstNode *field_name = field->names[fj];
					ast_node(field_ident, Ident, field_name);
					if(field_ident->token.string[0] == '_') {
						continue;
					}

					gb_fprintf(doc_file, "%.*s ", LIT(field_ident->token.string));
				}
				gb_fprintf(doc_file, " | ");

				if (field->type != nullptr) {
					gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
					print_anchor(doc_file, field->type);
					gb_fprintf(doc_file, "\">");
					print_type(doc_file, field->type);
					gb_fprintf(doc_file, "</a>");
				}
				gb_fprintf(doc_file, "\n");
		}
	} 	

	gb_fprintf(doc_file, "\n\n");

}

void print_enum_type(gbFile *doc_file, AstNode *name, AstNode *type, CommentGroup docs, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_EnumType);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}
	ast_node(et, EnumType, type);
	gb_fprintf(doc_file, "## %.*s\n\n", LIT(ident->token.string));
	gb_fprintf(doc_file, "Location: %.*s:%d\n\n", LIT(ident->token.pos.file), ident->token.pos.line);
	gb_fprintf(doc_file, "| Name | Value | Desc |\n");
	gb_fprintf(doc_file, "|-|-|-|\n");

	for_array(fi, et->fields) {
		AstNode *f = et->fields[fi];
		switch (f->kind) {
			case AstNode_Ident :
				print_type(doc_file, f);
				gb_fprintf(doc_file, " | ");
				gb_fprintf(doc_file, " | ");
				gb_fprintf(doc_file, "\n");
				break;

			case_ast_node(fv, FieldValue, f)
				print_type(doc_file, fv->field);
				gb_fprintf(doc_file, " | ");
				gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
				print_anchor(doc_file, fv->value);
				gb_fprintf(doc_file, "\">");
				print_type(doc_file, fv->value);
				gb_fprintf(doc_file, "</a>");
				gb_fprintf(doc_file, " | ");
				gb_fprintf(doc_file, "\n");
			case_end;
		}

	}
	gb_fprintf(doc_file, "\n\n");
}

void print_const_type(gbFile *doc_file, AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}
	//String docstr = alloc_comment_group_string(heap_allocator(), docs);
	//gb_fprintf(doc_file, "(%.*s:%d)\n", LIT(ident->token.pos.file), ident->token.pos.line);
	//gb_fprintf(doc_file, "%.*s", LIT(docstr));
	gb_fprintf(doc_file, "`%.*s :: ", LIT(ident->token.string));
	print_type(doc_file, type);
	gb_fprintf(doc_file, "`\n\n");
}

void print_union_type(gbFile *doc_file, AstNode *name, AstNode *type, bool skip_hidden) {
	GB_ASSERT(name->kind == AstNode_Ident);
	GB_ASSERT(type->kind == AstNode_UnionType);
	ast_node(ident, Ident, name);
	if(ident->token.string[0] == '_' && skip_hidden) {
		return;
	}
	ast_node(ut, UnionType, type);

	gb_fprintf(doc_file, "## %.*s\n", LIT(ident->token.string));

	gb_fprintf(doc_file, "```go\n");
	gb_fprintf(doc_file, "%.*s :: union {\n", LIT(ident->token.string));
	for_array(var_index, ut->variants) {
		AstNode *variant = ut->variants[var_index];
		gb_fprintf(doc_file, "    ");
		print_type(doc_file, variant);
		if(var_index != ut->variants.count-1) {
			gb_fprintf(doc_file, ", ");
		}
		gb_fprintf(doc_file, "\n");
	}
	gb_fprintf(doc_file, "}\n```\n");

	//Markdown
	gb_fprintf(doc_file, "Type: Union\n\n");
	gb_fprintf(doc_file, "Location: %.*s:%d\n\n", LIT(ident->token.pos.file), ident->token.pos.line);
	gb_fprintf(doc_file, "| Variant | Desc |\n");
	gb_fprintf(doc_file, "|-|-|\n");
	for_array(var_index, ut->variants) {
		gb_fprintf(doc_file, "|");
		AstNode *variant = ut->variants[var_index];
		gb_fprintf(doc_file, "<a style=\"color:inherit; text-decoration:inherit\" href=\"");
		print_anchor(doc_file, variant);
		gb_fprintf(doc_file, "\">");
		print_type(doc_file, variant);
		gb_fprintf(doc_file, "</a>");
		gb_fprintf(doc_file, "|");
		gb_fprintf(doc_file, " |");
		gb_fprintf(doc_file, "\n");
	}
}

void print_declaration(gbFile *doc_file, AstNode *decl) {
	switch (decl->kind) {
		case_ast_node(vd, ValueDecl, decl);
			isize max = gb_min(vd->names.count, vd->values.count);
			for (isize i = 0; i < max; i++) {
				AstNode *name = vd->names[i];
				AstNode *value = vd->values[i];
				switch (value->kind) {
					case AstNode_StructType : 
						print_struct_type(doc_file, name, value, vd->docs, true);
						break;
					case AstNode_EnumType :
						print_enum_type(doc_file, name, value, vd->docs, true);
						break;
					case_ast_node(lit, ProcLit, value);
						print_proc_type(doc_file, name, lit->type, false, true);
					case_end;

					case_ast_node(help, HelperType, value);
						print_proc_type(doc_file, name, help->type, true, true); 
					case_end;

					case AstNode_BasicLit :
					case AstNode_BinaryExpr :
					case AstNode_UnaryExpr :
					case AstNode_Ident :
					case AstNode_SelectorExpr :
					case AstNode_DynamicArrayType :
						print_const_type(doc_file, name, value, true); 
						break;

					case AstNode_ProcGroup : 
						print_proc_group(doc_file, name, value, true);
						break;

					case AstNode_UnionType :
						print_union_type(doc_file, name, value, true);
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
	gbFile doc_file = {};
	char doc_name[1024];
	gb_snprintf(doc_name, 1024, "%.*s.html.md", LIT(filename_from_path(fullpath)));
	gb_file_create(&doc_file, doc_name);
	gb_printf("------ Parsing: %.*s ------\n", LIT(fullpath));
#if 0
	gb_fprintf(&doc_file, "---\n");
	gb_fprintf(&doc_file, "title: %.*s API Reference\n",  LIT(filename_from_path(fullpath)));
	gb_fprintf(&doc_file, "\n");
	gb_fprintf(&doc_file, "language_tabs:\n");
	gb_fprintf(&doc_file, "  - odin\n");
	gb_fprintf(&doc_file, "\n");
	gb_fprintf(&doc_file, "toc_footers:\n");
	gb_fprintf(&doc_file, "  - <a href='https://github.com/odin-lang/odin'>Generated by the Odin Compiler</a>\n");
	gb_fprintf(&doc_file, "  - <a href='https://github.com/lord/slate'>Documentation Powered by Slate</a>\n");
	gb_fprintf(&doc_file, "\n");
	gb_fprintf(&doc_file, "search: true\n");
	gb_fprintf(&doc_file, "---");
	gb_fprintf(&doc_file, "\n");
	gb_fprintf(&doc_file, "# Notice\n");
	gb_fprintf(&doc_file, "\n");
	gb_fprintf(&doc_file, "This is a test for auto generating documentation from odin code via the Odin compiler.\n\n");
#endif

	//Collect Decls
	Array<AstNode *> type_decls = make_ast_node_array(file);
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
				document_file(id->file);
			case_end;

			case_ast_node(ed, ExportDecl, decl);
				if (ed->file == nullptr) {
					break;
				}
				document_file(ed->file);
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
		gb_fprintf(&doc_file, "# %.*s - Constants\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, const_decls) {
			AstNode *decl = const_decls[decl_index];
			print_declaration(&doc_file, decl);
		}
	}
	if(type_decls.count > 0) {
		gb_fprintf(&doc_file, "# %.*s - Types\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, type_decls) {
			AstNode *decl = type_decls[decl_index];
			print_declaration(&doc_file, decl);
		}
	}
	if(proc_decls.count > 0) {
		gb_fprintf(&doc_file, "# %.*s - Procs\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, proc_decls) {
			AstNode *decl = proc_decls[decl_index];
			print_declaration(&doc_file, decl);
		}
	}
	if(enum_decls.count > 0) {
		gb_fprintf(&doc_file, "# %.*s - Enums\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, enum_decls) {
			AstNode *decl = enum_decls[decl_index];
			print_declaration(&doc_file, decl);
		}
	}
	gb_printf("------ Done! ------\n\n");
	gb_file_close(&doc_file);
	gb_fprintf(&doc_file, "------------------------------------\n");
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

// Generates Documentation

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

void print_type(gbFile *doc_file, AstNode *node) {
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

				gb_fprintf(doc_file, "   %.*s", LIT(field_ident->token.string));
			}
			
			if(skip) {
				continue;
			}

			if (field->type != nullptr) {
				gb_fprintf(doc_file, " : ");
				print_type(doc_file, field->type);
				gb_fprintf(doc_file, "\n");
			} else {
				gb_fprintf(doc_file, " := ");
				print_type(doc_file, field->default_value);
				gb_fprintf(doc_file, "\n");
			}
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
				print_type(doc_file, field->type);
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
				gb_fprintf(doc_file, "   %.*s ", LIT(field_ident->token.string));
			}

			if (field->type != nullptr) {
				gb_fprintf(doc_file, ": ");
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
				gb_fprintf(doc_file, "%.*s ", LIT(field_ident->token.string));
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
						print_const_type(doc_file, name, value, true); 
						break;

					case AstNode_ProcGroup : 
						print_proc_group(doc_file, name, value, true);
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
	gb_snprintf(doc_name, 1024, "%.*s.md", LIT(filename_from_path(fullpath)));
	gb_file_create(&doc_file, doc_name);
	gb_printf("Parsing: %.*s... \n", LIT(fullpath));

	//Collect Decls
	Array<AstNode *> struct_decls = make_ast_node_array(file);
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
						case AstNode_StructType : 
							array_add(&struct_decls, decl);
							break;
						case AstNode_ProcGroup :
						case AstNode_ProcLit : 
							array_add(&proc_decls, decl);
							break;
						case AstNode_EnumType : 
							array_add(&enum_decls, decl);
							break;
						case AstNode_HelperType :
							array_add(&helper_decls, decl);
							break;	
						
						case AstNode_BasicLit :
						case AstNode_BinaryExpr :
						case AstNode_UnaryExpr :
						case AstNode_Ident : //typedef, probably should be handled by itself (check for alias and such)
						case AstNode_SelectorExpr :
							array_add(&const_decls, decl);
							break;

						default : 
							gb_printf("\nVALUE DECL NOT HANDLED: %.*s\n", LIT(ast_node_strings[value->kind]));
							break;
					}
				}
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
	if(struct_decls.count > 0) {
		gb_fprintf(&doc_file, "# %.*s - Structs\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, struct_decls) {
			AstNode *decl = struct_decls[decl_index];
			print_declaration(&doc_file, decl);
		}
	}
	if(proc_decls.count > 0 || helper_decls.count > 0) {
		gb_fprintf(&doc_file, "# %.*s - Procs\n\n", LIT(filename_from_path(fullpath)));
		for_array(decl_index, proc_decls) {
			AstNode *decl = proc_decls[decl_index];
			print_declaration(&doc_file, decl);
		}

		for_array(decl_index, helper_decls) {
			AstNode *decl = helper_decls[decl_index];
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
	gb_printf("Done!\n");
	gb_file_close(&doc_file);
	gb_fprintf(&doc_file, "------------------------------------\n");
}

void generate_documentation(Parser *parser) {
	for_array(file_index, parser->files) {
		AstFile *file = parser->files[file_index];
		document_file(file);
	}
}

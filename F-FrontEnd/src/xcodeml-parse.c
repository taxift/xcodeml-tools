#include "xcodeml.h"
#include "F-front-context.h"

static const char *sanitizeText(const char *str)
{
    const char *p = str;

    bool foundNonSpaces = false;
    while (*p != '\0') {
        if (isspace((int)(*p))) {
            p++;
        } else {
            foundNonSpaces = true;
            break;
        }
    }
    if (foundNonSpaces == true) {
        return str;
    } else {
        return "";
    }
}

static bool xcParse(xmlNode *ndPtr, const char *fileName, XcodeMLNode **pnPtr)
{
    bool ret = false;

    if (ndPtr != NULL) {
        bool isComment = false;
        xmlNode *curNode;
        XcodeMLNode *me = NULL;

        for (curNode = ndPtr; curNode != NULL; curNode = curNode->next) {

            me = NULL;
            isComment = false;

            switch (curNode->type) {

                case XML_ELEMENT_NODE: {
                    me = xcodeml_CreateList0(XcodeML_Element);
                    XCODEML_NAME(me) = strdup((const char*)curNode->name);
                    break;
                }

                case XML_ATTRIBUTE_NODE: {
                    me = xcodeml_CreateList0(XcodeML_Attribute);
                    XCODEML_NAME(me) = strdup((const char*)curNode->name);
                    break;
                }

                case XML_TEXT_NODE: {
                    const char *text = sanitizeText((const char*)curNode->content);
                    me = xcodeml_CreateValueNode(text);
                    break;
                }

                case XML_COMMENT_NODE: {
                    isComment = true;
                    break;
                }

                default: {
                    fprintf(stderr,
                            "%s: Error: unknown node type "
                            "%d.\n",
                            fileName, curNode->type);
                    break;
                }
            }

            if (isComment == true) {
                continue;
            }

            if (me == NULL) {
                fprintf(stderr, "%s: Error: Can't analyze current node.\n",
                        fileName);
                break;
            }

            if (curNode->children == NULL && curNode->properties == NULL) {
                goto AddMe;
            } else if (XCODEML_TYPE(me) == XcodeML_Value) {
                fprintf(stderr, "%s: Error: a value node has child.\n",
                        fileName);
                break;
            }

            if (curNode->properties != NULL) {
                if (xcParse((xmlNode *)(curNode->properties), fileName, &me) ==
                    false) {
                    fprintf(stderr, "%s: XML error: properties error(s).\n",
                            fileName);
                    break;
                }
            }
            if (curNode->children != NULL) {
                if (xcParse(curNode->children, fileName, &me) == false) {
                    fprintf(stderr, "%s: XML error: children error(s).\n",
                            fileName);
                    break;
                }
            }

        AddMe:
            if (*pnPtr == NULL) {
                *pnPtr = me;
                continue;
            }
            *pnPtr = xcodeml_AppendNode(*pnPtr, me);
        }
        if (curNode == NULL) {
            ret = true;
        }
    } else {
        ret = true;
    }

    return ret;
}

const char *xcodeml_GetAttributeValue(XcodeMLNode *ndPtr)
{
    if (XCODEML_TYPE(ndPtr) == XcodeML_Attribute) {
        if (XCODEML_TYPE(XCODEML_ARG1(ndPtr)) == XcodeML_Value) {
            return XCODEML_VALUE(XCODEML_ARG1(ndPtr));
        }
    }
    return "";
}

const char *xcodeml_GetElementValue(XcodeMLNode *ndPtr)
{
    if (XCODEML_TYPE(ndPtr) == XcodeML_Element) {
        XcodeMLNode *x1;
        XcodeMLList *lp;
        FOR_ITEMS_IN_XCODEML_LIST (lp, ndPtr) {
            x1 = XCODEML_LIST_NODE(lp);
            if (XCODEML_TYPE(x1) == XcodeML_Value) {
                return XCODEML_VALUE(x1);
            }
        }
    }
    return "";
}

#include "F-front.h"

XcodeMLNode *xcodeml_ParseFile(const char *fileName)
{
    sds_string buff = sdsempty();

    xmlDocPtr doc;
    const size_t includeDirvI = vector_size(inc_dir_paths());
    const size_t modincludeDirvI = vector_size(xmod_inc_dir_paths());
    char** const includeDirv = (char** const)inc_dir_paths();
    char** const modincludeDirv = (char** const)xmod_inc_dir_paths();
    int i;

    XcodeMLNode *ret = NULL;
    xmlNode *rootNode = NULL;
    bool succeeded = false;

    {
        void* startAddress = NULL;
        size_t size = 0;

          if(ctx->files_cache && io_cache_get_input_file_as_mem(ctx->files_cache, fileName, &startAddress, &size))
        {
              doc = xmlReadMemory((const char *)startAddress, size, NULL, NULL,
                              XML_PARSE_NOBLANKS | XML_PARSE_NONET | XML_PARSE_NOWARNING);
        }
        else
        {
            doc = xmlReadFile(fileName, NULL,
                                          XML_PARSE_NOBLANKS | XML_PARSE_NONET | XML_PARSE_NOWARNING);
        }
    }

    if (!doc) {

        if (modincludeDirvI > 0) {
            set_sds_string(&buff, modincludeDirv[0]);
            buff = sdscat(buff, "/");
            buff = sdscat(buff, fileName);

            doc = xmlReadFile(buff, NULL,
                              XML_PARSE_NOBLANKS | XML_PARSE_NONET |
                                  XML_PARSE_NOWARNING);
        }
    }

    if (!doc) {

        for (i = 0; i < includeDirvI; i++) {
            set_sds_string(&buff, includeDirv[i]);
            buff = sdscat(buff, "/");
            buff = sdscat(buff, fileName);

            doc = xmlReadFile(buff, NULL,
                              XML_PARSE_NOBLANKS | XML_PARSE_NONET |
                                  XML_PARSE_NOWARNING);
            if (doc)
                break;
        }
    }

    if (doc == NULL) {
        fprintf(stderr, "Can't parse \"%s\".\n", fileName);
        goto Done;
    }

    rootNode = xmlDocGetRootElement(doc);
    if (rootNode == NULL) {
        fprintf(stderr, "Can't get a root node of \"%s\".\n", doc->name);
        goto Done;
    }

    succeeded = xcParse(rootNode, fileName, &ret);
    if (succeeded == false) {
        ret = NULL;
    }

Done:
    sdsfree(buff);
    if (doc != NULL) {
        (void)xmlFreeDoc(doc);
    }

    return ret;
}

static void lvlFprintf(int lvl, FILE *fd, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (lvl > 0) {
        char lvlFmt[1024];
        snprintf(lvlFmt, 1024, "%%%ds", lvl);
        fprintf(fd, lvlFmt, "");
    }

    vfprintf(fd, fmt, args);
    va_end(args);
}

void xcodeml_DumpTree(FILE *fd, XcodeMLNode *ndPtr, int lvl)
{
    lvlFprintf(lvl, fd, "(");

    switch (XCODEML_TYPE(ndPtr)) {
        case XcodeML_Value: {
            fprintf(fd, "value \"%s\")", XCODEML_VALUE(ndPtr));
            break;
        }

        case XcodeML_Element: {
            const char *v = xcodeml_GetElementValue(ndPtr);
            XcodeMLNode *x1;
            XcodeMLList *lp;

            if (strlen(v) > 0) {
                fprintf(fd, "TAG (\"%s\" \"%s\")", XCODEML_NAME(ndPtr), v);
            } else {
                fprintf(fd, "TAG (\"%s\")", XCODEML_NAME(ndPtr));
            }

            FOR_ITEMS_IN_XCODEML_LIST (lp, ndPtr) {
                x1 = XCODEML_LIST_NODE(lp);
                if (XCODEML_TYPE(x1) != XcodeML_Value) {
                    fprintf(fd, "\n");
                    xcodeml_DumpTree(fd, x1, lvl + 1);
                }
            }
            fprintf(fd, ")");

            break;
        }

        case XcodeML_Attribute: {
            const char *v = xcodeml_GetAttributeValue(ndPtr);
            if (strlen(v) > 0) {
                fprintf(fd, "PROP (\"%s\" \"%s\")", XCODEML_NAME(ndPtr), v);
            } else {
                fprintf(fd, "PROP (\"%s\")", XCODEML_NAME(ndPtr));
            }
            fprintf(fd, ")");
            break;
        }

        default: {
            fprintf(fd, "UNKNOWN)");
            break;
        }
    }

    if (lvl == 0) {
        fprintf(fd, "\n");
    }

    return;
}

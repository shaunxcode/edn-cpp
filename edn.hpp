#include <list>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
namespace edn { 
  using std::cout;
  using std::endl;
  using std::string;
  using std::list;

  enum TokenType {
    TokenString,
    TokenAtom,
    TokenParen
  };

  struct EdnToken {
    TokenType type;
    int line;
    string value;
  };

  enum NodeType {
    EdnNil,
    EdnSymbol,
    EdnKeyword,
    EdnBool,
    EdnInt,
    EdnFloat,
    EdnString, 
    EdnChar, 

    EdnList,
    EdnVector,
    EdnMap,
    EdnSet,

    EdnDiscard,
    EdnTagged
  };

  struct EdnNode {
    NodeType type;
    int line;
    string value;
    list<EdnNode> values;
  };

  void createToken(TokenType type, int line, string value, list<EdnToken> &tokens) {
    EdnToken token;
    token.type = type;
    token.line = line;
    token.value = value;
    tokens.push_back(token);
  }


  list<EdnToken> lex(string edn) {
    string::iterator it;
    int line = 1;
    char escapeChar = '\\';
    bool escaping = false;
    bool inString = false;
    string stringContent = "";
    bool inComment = false;
    string token = "";
    string paren = "";
    list<EdnToken> tokens;

    for (it = edn.begin(); it != edn.end(); ++it) {
      if (*it == '\n' || *it == '\r') line++;

      if (!inString && *it == ';' && !escaping) inComment = true;

      if (inComment) {
        if (*it == '\n') {
          inComment = false;
          if (token != "") {
              createToken(TokenAtom, line, token, tokens);
              token = "";
          }
          continue;
        }
      }
    
      if (*it == '"' && !escaping) {
        if (inString) {
          createToken(TokenString, line, stringContent, tokens);
          inString = false;
        } else {
          stringContent = "";
          inString = true;
        }
        continue;
      }

      if (inString) { 
        if (*it == escapeChar && !escaping) {
          escaping = true;
          continue;
        }

        if (escaping) {
          escaping = false;
          if (*it == 't' || *it == 'n' || *it == 'f' || *it == 'r') stringContent += escapeChar;
        }
        stringContent += *it;   
      } else if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || 
                 *it == '}' || *it == '\t' || *it == '\n' || *it == '\r' || * it == ' ') {
        if (token != "") { 
          createToken(TokenAtom, line, token, tokens);
          token = "";
        } 
        if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || *it == '}') {
          paren = "";
          paren += *it;
          createToken(TokenParen, line, paren, tokens);
        }
      } else {
        if (escaping) { 
          escaping = false;
        } else if (*it == escapeChar) {
          escaping = true;
        }

        if (token == "#_") {
          createToken(TokenAtom, line, token, tokens); 
          token = "";
        }
        token += *it;
      }
    }
    if (token != "") {
      createToken(TokenAtom, line, token, tokens); 
    }

    return tokens;
  }

  bool validSymbol(string value) {
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    if (std::strspn(value.c_str(), "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.*+!-_?$%&=:#/") != value.length()) { 
      cout << "NOT A SYMBOL" << endl;
      return false;
    }
    return true;
  }

  bool validNil(string value) {
    return (value == "nil");
  }

  bool validBool(string value) {
    return (value == "true" || value == "false");
  }

  bool validInt(string value) {
    return false;
  }

  bool validFloat(string value) {
    return false;
  }

  bool validChar(string value) {
    return (value.at(0) == '\\' && value.length() == 2);
  }

  EdnNode handleAtom(EdnToken token) {
    EdnNode node;
    node.type = EdnSymbol; // just for debug until rest of valid funcs are working
    node.line = token.line;
    node.value = token.value;

    if (validNil(token.value)) { 
      node.type = EdnNil;
    } else if (token.type == TokenString) { 
      node.type = EdnString;
    } else if (validSymbol(token.value)) { 
      if (token.value.at(0) == ':') {
        node.type = EdnKeyword;
      } else {
        node.type = EdnSymbol; 
      }
    }
    else if (validChar(token.value)) {
      node.type = EdnChar;
    }
    else if (validBool(token.value)) { 
      node.type = EdnBool;
    }
    else if (validInt(token.value)) {
      node.type = EdnInt;
    }
    else if (validFloat(token.value)) {
      node.type = EdnFloat;
    } 

    return node;
  }

  EdnNode handleCollection(EdnToken token, list<EdnNode> values) {
    EdnNode node;
    node.line = token.line;
    node.values = values;

    if (token.value == "(") {
      node.type = EdnList;
    }
    else if (token.value == "[") {
      node.type = EdnVector;
    }
    if (token.value == "{") {
      node.type = EdnMap;
    }
    return node;
  }

  EdnNode handleTagged(EdnToken token, EdnNode value) { 
    EdnNode node;
    node.line = token.line;

    string tagName = token.value.substr(1, token.value.length() - 1);
    if (tagName == "_") {
      node.type = EdnDiscard;
    } else if (tagName == "") {
      //special case where we return early as # { is a set - thus tagname is empty
      node.type = EdnSet;
      if (value.type != EdnMap) {
        throw "Was expection a { } after hash to build set";
      }
      node.values = value.values;
      return node;
    } else {
      node.type = EdnTagged;
    }
  
    if (!validSymbol(tagName)) {
      throw "Invalid tag name at line: "; 
    }

    EdnToken symToken;
    symToken.type = TokenAtom;
    symToken.line = token.line;
    symToken.value = tagName;
  
    list<EdnNode> values;
    values.push_back(handleAtom(symToken)); 
    values.push_back(value);

    node.values = values;
    return node; 
  }

  EdnToken shiftToken(list<EdnToken> &tokens) { 
    EdnToken nextToken = tokens.front();
    tokens.pop_front();
    return nextToken;
  }

  EdnNode readAhead(EdnToken token, list<EdnToken> &tokens) {
    if (token.type == TokenParen) {

      EdnToken nextToken;
      list<EdnNode> L;
      string closeParen;
      if (token.value == "(") closeParen = ")";
      if (token.value == "[") closeParen = "]"; 
      if (token.value == "{") closeParen = "}"; 

      while (true) {
        if (tokens.empty()) throw "unexpected end of list";

        nextToken = shiftToken(tokens);

        if (nextToken.value == closeParen) {
          return handleCollection(token, L);
        } else {
          L.push_back(readAhead(nextToken, tokens));
        }
      }
    } else if (token.value == ")" || token.value == "]" || token.value == "}") {
      throw "Unexpected " + token.value;
    } else {
      if (token.value.at(0) == '#') {
        return handleTagged(token, readAhead(shiftToken(tokens), tokens));
      } else {
        return handleAtom(token);
      }
    }
  }

  string pprint(EdnNode node) {
    string output;
    if (node.type == EdnList || node.type == EdnSet || node.type == EdnVector || node.type == EdnMap) { 
      string vals = "";
      for (list<EdnNode>::iterator it=node.values.begin(); it != node.values.end(); ++it) {
        if (vals.length() > 0) { 
          vals += " ";
        } 
        vals += pprint(*it);
      }

      if (node.type == EdnList) output = "(" + vals + ")";
      else if (node.type == EdnMap) output = "{" + vals + "}";
      else if (node.type == EdnVector) output = "[" + vals + "]"; 
      
    } else if (node.type == EdnTagged) { 
      output = "#" + pprint(node.values.front()) + " " + pprint(node.values.back());
    } else if (node.type == EdnString) {
      output = "\"" + node.value + "\"";
    } else {
      output = node.value;
    }
    return output;
  }

  EdnNode read(string edn) {
    list<EdnToken> tokens = lex(edn);
    
    if (tokens.size() == 0) {
      throw "No parsable tokens found in string";
    }

    return readAhead(shiftToken(tokens), tokens);
  }
}

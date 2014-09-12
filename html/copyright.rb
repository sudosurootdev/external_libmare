# encoding: UTF-8


class String

  def XML_close_bracket
    case self
      when /!--/
        "-->"
      when /!\[CDATA\[/
        "]]>"
      when /\?.+/
        "?>"
      else
        ">"
    end
  end  # XML_close_bracket
  
  def csv
      temp = String.new(self)
      temp.gsub!(/^(=|-|\+|@|\')/, ' \1')  # for Excel-- to avoid interpreting as a function by adding initial space char
      if !temp.empty? && temp[0,1] != '"' && (temp.index(",") || temp.index("\n")) then
        temp.gsub!('"', '""')  # double up double-quotes
        temp = '"' + temp + '"'
      end
    return temp
  end  # csv

  def translate_entities
    result = self.dup
    result.gsub!(/&(\S+?);/) do |x|
      case $1
        when "amp" then "&"
        when "apos" then "'"
        when "gt" then ">"
        when "lt" then "<"
        when "quot" then "\""
        when "#091" then "["
        when "#093" then "]"
        when "#xA0" then " "  #nbsp
        when "#x2011" then "-"
        when "#x2409", "#x9" then "\t"
        when "#x240D" then "\n"
        else
          $&
      end
    end
    return result
  end  # translate_entities
  
  def less_entities
    result = self.dup
    result.gsub!(/&(\S+?);/) do |x|
      case $1
        when "amp" then "&"
        when "apos" then "'"
        when "gt" then ">"
        when "lt" then "<"
        when "quot" then "\""
        when "#x2011" then "-"
        when "#x2409", "#x9" then "\t"
        when "#x240D" then "\n"
        when "#x200B" then ""
        when "#xA0" then " "  #nbsp
        else
          " "
      end
    end
    return result
  end  # less_entities
  
  def to_entities
    result = String.new(self)
    result.gsub!(/\[|\<|\]|\>/) do |s|
      case $&
        when "[" then "&#091;"
        when "<" then "&lt;"
        when "]" then "&#093;"
        when ">" then "&gt;"
      end
    end
    return result
  end  # to_entities
  
  def from_xml
    result = String.new(self)
    result.gsub!("&amp;", "&")
    result.gsub!("&gt;", ">")
    result.gsub!("&lt;", "<")
    result.gsub!("&quot;", "\"")
    result.gsub!("&apos;", "'")
    result.gsub!("&#x9;", "\t")
    result.gsub!("&#013;", "\n")
    return result
  end  # from_xml
  
end  # String


class Element

  attr_accessor :tag
  attr_accessor :attrs
  attr_accessor :pcdata

  def initialize(tag = "", attrs = Hash.new, pcdata = "")
    @tag = tag
    @attrs = attrs
    @pcdata = pcdata
  end
  
  def copy
    return Element.new(String.new(@tag), Hash.new.replace(@attrs), String.new(@pcdata))
  end

  def to_s
    str = String.new
    str << @tag
    @attrs.each_pair {|key, value| str << ' ' + key.to_s + '="' + value.to_s + '"' }
    str << @pcdata
    return str  # .force_encoding("UTF-8")
  end
 
  def to_re  # to regular expression
  end
   
 
  def matches?(str="")
    # convert Element to regular expression return true if it matches string 'str'
    # allows for Element @tag to be prefaced by '+' '-' '*' or '$' 
  end  # matches

end  #Element


class IXML
  
  attr_accessor :element
  attr_accessor :child
  
  def initialize(e=Element.new, c=[])
    @child = c
    @element = e
  end  # initialize
  
  def copy
    children = []
    @child.each {|c| children << c.copy }
    return IXML.new(@element.copy, children)
  end  # copy
  
  def sub(tag, attrs = Hash.new, pcdata = String.new)
    x = IXML.new(Element.new(self.element.tag))
    x.element.attrs.replace(self.element.attrs)
    x.element.pcdata = "" << self.element.pcdata
    x.child += self.child
    self.element.tag = tag
    self.element.attrs.replace(attrs)
    self.element.pcdata = pcdata
    self.child = [x]
  end  # sub
  
  def insertChild(e=nil)
    x = IXML.new(e)
    @child << x
    return x
  end  # insertChild
  
  def insertChildBefore(e=nil, eWhere=Element.new)
    x = IXML.new(e)
    index = 0  # first by default
    @child.each_index do |i| 
      if @child[i].element.to_s =~ eWhere.regExp then
        index = i 
        break
      end
    end
    @child.insert(index, x)
    return x
  end  # insertChildBefore
  
  
  def insertAt(subTree, context)
    # 'context' is array of strings
    ptr = self
    until context.empty? || ptr.child.empty? || ptr.child.last.element.tag != context.first do
      context.shift
      ptr = ptr.child.last
    end
    context.each do |c| 
      temp = IXML.new
      temp.element.tag = c
      ptr.child << temp
      ptr = temp
    end
    ptr.child << subTree
  end  # insertAt
  
  
  def insertPCDataAt(str, context)
    # 'context' is array of array of string, hash
    ptr = self
    until context.empty? || ptr.child.empty? || ptr.child.last.element.tag != context.first.first do
      context.shift
      ptr = ptr.child.last
    end
    context.each do |c, h| 
      temp = IXML.new
      temp.element.tag = c
      temp.element.attrs = h if h
      ptr.child << temp
      ptr = temp
    end
    ptr.element.pcdata << String.new(str)
  end  # insertPCDataAt
  
  
  def insertAtEnd(subTree, tag, context, h = {}, h2 = {})
    # 'context' is array of array of strings
    # combo: look for the last tag, create it if not found, then create context as nec. and insert there...
    ptr = self
    here = ptr if ptr.element.tag == tag
    until ptr.child.empty? do
      here = ptr.child.last if ptr.child.last.element.tag == tag
      ptr = ptr.child.last
    end
    if !here then
      temp = IXML.new
      temp.element.tag = tag
      self.child << temp
      here = temp
    end
    
    ptr = here
    until context.empty? || ptr.child.empty? || !context.first.include?(ptr.child.last.element.tag) do
      context.shift
      ptr = ptr.child.last
    end
    created = false
    context.each do |c| 
      temp = IXML.new
      created = true
      temp.element.tag = c.first
      ptr.child << temp
      ptr = temp
    end
    ptr.element.attrs.merge!(h) unless h.empty?
    ptr.element.attrs.merge!(h2) if created && !h2.empty?
    ptr.child << subTree
  end  # insertAtEnd
  
  
  def insertAtLast(subTree, tag, h = {})
    ptr = self
    here = ptr if ptr.element.tag == tag
    until ptr.child.empty? do
      here = ptr.child.last if ptr.child.last.element.tag == tag
      ptr = ptr.child.last
    end
    if here then
      here.child << subTree
    else
      temp = IXML.new
      temp.element.tag = tag
      temp.element.attrs.merge!(h) unless h.empty?
      temp.child << subTree
      self.child << temp
    end
  end  # insertAtLast
  
  
  def deleteAtLast(match_tag, tag, tag2 = nil)
    ptr = self
    here = ptr if ptr.element.tag == tag
    until ptr.child.empty? do
      here = ptr.child.last if ptr.child.last.element.tag == tag
      ptr = ptr.child.last
    end
    if tag2 && here then
      ptr = here
      until ptr.child.empty? do
        here = ptr.child.last if ptr.child.last.element.tag == tag2
        ptr = ptr.child.last
      end
    end
    if here then
      i = here.child.length - 1
      while i >= 0 do
        if here.child[i].element.tag == match_tag then
          return here.child.slice!(i)
        end
        i -= 1
      end
    end
    return nil
  end  # deleteAtLast
  
  
  def unshiftAtLast(subTree, tag, h = {})
    ptr = self
    here = ptr if ptr.element.tag == tag
    until ptr.child.empty? do
      here = ptr.child.last if ptr.child.last.element.tag == tag
      ptr = ptr.child.last
    end
    if here then
      here.child.unshift(subTree)
    else
      temp = IXML.new
      temp.element.tag = tag
      temp.element.attrs.merge!(h) unless h.empty?
      temp.child.unshift(subTree)
      self.child << temp
    end
  end  # unshiftAtLast
  
  
  def insertAtBottom(subTree)
    ptr = self
    until ptr.child.empty? || ptr.child.last.element.tag.empty? do
      ptr = ptr.child.last
    end
    ptr.child << subTree
  end  # insertAtBottom
  
  
  def insertTxt(str, tag=nil, type=nil)
    if tag then
      a = IXML.new(Element.new(tag, (type ? {"type" => type} : {}), "")) 
      a.child << IXML.new(Element.new("", {}, str))  # pcdata
    else
      a = IXML.new(Element.new("", {}, str))  # pcdata
    end
    @child << a
  end  # insertTxt()
  
  
  def traverse()
    yield @element
    @child.each {|c| c.traverse {|e| yield(e)} }
  end  # traverse


  def walk()
    yield self
    @child.each {|c| c.walk {|c| yield c } }
  end  # walk
  
  @@level = 0
  def walkl(trigger_tags=["section"], &block)
    is_trigger = false
    trigger_tags.each do |tt|
      if @element.tag == tt then
        is_trigger = true
        @@level += 1 
        break
      end
    end
    yield(self, @@level)
    @child.each {|c| c.walkl(trigger_tags, &block) }
    @@level -= 1 if is_trigger
  end  # walkl
  
  
  @@context = []
  def walkc(&block)
    if @element.tag == "div" && @element.attrs["class"] == "line" then 
      @@context << "code"
    elsif @element.attrs["class"] == "formulaDsp" then
      @@context << "code"
    else
      @@context << @element.tag
    end
    #~ @@context << (@element.tag == "div" ? (@element.attrs["class"] == "line" ? "code" : "div") : @element.tag)
    yield(self, @@context)
    @child.each {|c| c.walkc(&block) }
    @@context.pop
  end  # walkc
  

  def to_s()
    str = ""
    if @element.tag.empty? then
      str << @element.to_s
      @child.each {|c| str << c.to_s }
    else
      str << "<" + @element.to_s
      if @child.empty? then
        str << "/>\n"
      else
        str << ">"
        @child.each {|c| str << c.to_s }
        str << "</" + @element.tag + ">\n"
      end
    end
    return str
  end  # to_s
 
 
  def get_pcdata(sep = "\n", sep_tag = /^p(ara)?$/, xlate = true)
    buffer = ""
    buffer << @element.pcdata
    @child.each do |x|
      buffer << x.get_pcdata(sep, sep_tag, xlate)
    end
    buffer << sep if @element.tag =~ sep_tag
    if xlate then
      return buffer.from_xml
    else
      return buffer
    end
  end  # get_pcdata
  
  
  def char_count()
    result = (@element.tag.empty? ? @element.pcdata.less_entities.length + (@element.pcdata.count("A-Z")*1.4) : 0)
    result += (@element.tag == "xref" ? 12 : 0)
    @child.each {|c| result += c.char_count }
    return result
  end  # char_count


  @@level = -1
  @@sep = ""
  
  def writeIndentedXML(f = $stdout, stoptag = "deleteme")
    if @element.tag == stoptag then
      # do nothing
    elsif @element.tag.empty? then
      f.print @element.to_s
      @@sep = ""
      @child.each {|c| c.writeIndentedXML(f, stoptag) }
      @@sep = ""
    elsif @element.tag == "[" then  # internal DTD subset...
      f.print @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        f.print @@sep
        @@level.times {|i| f.print " " } unless @@sep.empty?
        @@sep = "\n"
      end
      f.print "]"
    elsif @element.tag == "!DOCTYPE" then
      @@level += 1
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        @@sep = "\n"
      end
      f.print @element.tag.XML_close_bracket 
      @@level -= 1
    elsif @element.tag == "!ENTITY" then
      @@level += 1
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        @@sep = "\n"
      end
      f.print @element.tag.XML_close_bracket 
      @@level -= 1
    else
      @@level += 1
      f.print @@sep
      @@level.times {|i| f.print " " } unless @@sep.empty?
      f.print "<" + @element.to_s
      @@sep = "\n"
      f.print " /" if @child.empty? && @element.pcdata.empty?
      f.print @element.tag.XML_close_bracket 
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        f.print @@sep
        @@level.times {|i| f.print " " } unless @@sep.empty?
        f.print "</" + @element.tag + @element.tag.XML_close_bracket 
        @@sep = "\n"
      end
      @@level -= 1
    end
  end  # writeIndentedXML
 
 
  def writeXML(f = $stdout, stoptag = "deleteme")
    if @element.tag == stoptag then
      # do nothing
    elsif @element.tag.empty? then
      #~ $stderr.puts "[#{@element.to_s.encoding}]#{@element.to_s}"  # debug
      f.print @element.to_s
      @@sep = ""
      @child.each {|c| c.writeXML(f, stoptag) }
      @@sep = ""
    elsif @element.tag == "pre" || @element.tag == "code" || @element.tag == "br" || 
          (@element.tag == "div" && @element.attrs["class"] == "line") ||
          (@element.attrs["class"] == "formulaDsp") then
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = ""
      f.print "/" if @child.empty? && @element.pcdata.empty?
      f.print @element.tag.XML_close_bracket 
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        f.print "</" + @element.tag + @element.tag.XML_close_bracket 
        @@sep = "\n"
      end
    elsif @element.tag == "["  then  # internal DTD subset...
      f.print @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        f.print @@sep
        @@sep = "\n"
      end
      f.print "]"
    elsif @element.tag == "!DOCTYPE" then
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        @@sep = "\n"
      end
      f.print @element.tag.XML_close_bracket 
    elsif @element.tag == "!ENTITY" then
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = "\n"
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        @@sep = "\n"
      end
      f.print @element.tag.XML_close_bracket 
    else
      f.print @@sep
      f.print "<" + @element.to_s
      @@sep = "\n"
      f.print @element.tag.XML_close_bracket + "</" + @element.tag if @child.empty? && @element.pcdata.empty?
      f.print @element.tag.XML_close_bracket 
      if !@child.empty? then
        @child.each {|c| c.writeXML(f, stoptag) }
        f.print @@sep
        f.print "</" + @element.tag + @element.tag.XML_close_bracket 
        @@sep = "\n"
      end
    end
  end  # writeXML
 
 
  def writeXML_ws(f = $stdout, stoptag = "deleteme", sep = "\n")
    if @element.tag == stoptag then
      # do nothing
    elsif @element.tag.empty? then
      #~ $stderr.puts "[#{@element.to_s.encoding}]#{@element.to_s}"  # debug
      f.print @element.to_s
      @child.each {|c| c.writeXML_ws(f, stoptag, "") }
    elsif @element.tag == "pre" || (@element.tag == "div" && @element.attrs["class"] == "line") || 
          (@element.tag == "span" && @element.attrs["class"] == "lineno") || 
          (@element.attrs["class"] == "formulaDsp") then
      f.print sep
      f.print "<" + @element.to_s + ">"
      @child.each {|c| c.writeXML_ws(f, stoptag, "")}
      f.print "</" + @element.tag + @element.tag.XML_close_bracket 
    elsif @element.tag == "code" || @element.tag == "br" then
      f.print sep
      f.print "<" + @element.to_s
      f.print "/" if @child.empty? && @element.pcdata.empty?
      f.print @element.tag.XML_close_bracket 
      if !@child.empty? then
        @child.each {|c| c.writeXML_ws(f, stoptag, "") }
        f.print "</" + @element.tag + @element.tag.XML_close_bracket 
      end
    elsif @element.tag == "["  then  # internal DTD subset...
      f.print @element.to_s
      if !@child.empty? then
        @child.each {|c| c.writeXML_ws(f, stoptag, "\n") }
        f.print "\n"  # sep
      end
      f.print "]"
    elsif @element.tag == "!DOCTYPE" then
      f.print "\n"  # sep
      f.print "<" + @element.to_s
      if !@child.empty? then
        @child.each {|c| c.writeXML_ws(f, stoptag, "\n") }
      end
      f.print @element.tag.XML_close_bracket 
    elsif @element.tag == "!ENTITY" then
      f.print "\n"  # sep
      f.print "<" + @element.to_s
      if !@child.empty? then
        @child.each {|c| c.writeXML_ws(f, stoptag, sep) }
      end
      f.print @element.tag.XML_close_bracket 
    else
      f.print sep
      f.print "<" + @element.to_s
      f.print @element.tag.XML_close_bracket + "</" + @element.tag if @child.empty? && @element.pcdata.empty?
      f.print @element.tag.XML_close_bracket 
      if !@child.empty? then
        @child.each {|c| c.writeXML_ws(f, stoptag, sep) }
        f.print sep unless (@child.length == 0 || (@child.length == 1 && @child[0].element.tag.empty?))
        f.print "</" + @element.tag + @element.tag.XML_close_bracket 
      end
    end
  end  # writeXML_ws

 
  def readXML(f = $stdin)
    qLT = 0
    qATT = 1
    qPCD = 2
    qCDATA = 3
    qDOCTYPE = 4
    qCOM = 5
    qDECL = 6
    qPI = 7
    qENTITY = 8
    qSCRIPT = 9
    
    curXML = []
    seeking = qLT
    line = f.gets
    done = FALSE
    begin
      case seeking 
        when qLT
          line << (f.gets || "") until line =~ /<\s*(!\[CDATA\[)|<(!--)|<\s*([^>\s][^>\s\[\/]*)/ || f.eof
          if $1 || $2 || $3 then
            tag = $1 || $2 || $3
            line = $'  # post-match
            case tag 
              when /!\[CDATA\[/
                seeking = qCDATA
                finis = /\]\]>/
              when /script/
                seeking = qSCRIPT
                finis = /<\/script>|\/>/
              when /!DOCTYPE/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qDOCTYPE
              when /!ENTITY/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qENTITY
              when /!--/
                #~ $stderr.puts line  # debug
                seeking = qCOM
                finis = /-->/
              when /!.+/
                seeking = qDECL
                finis = /\]>/
                #~ finis = />/
              when /\?.*/
                seeking = qPI
                finis = /\?>/
              else
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qATT
            end
          else
            done = TRUE
          end
          
        when qATT
          line << (f.gets || "") until line =~ /=|>/ || f.eof
          if $& == "=" then
            key = $`.strip  # pre-match
            line = $'  # post-match
            line << (f.gets || "") until line =~ /"|'/ || f.eof
            if !$&.empty? then
              temp = $&
              line << (f.gets || "") until f.eof || line.count(temp) >= 2
              m = line.match("#{temp}([^#{temp}]*)#{temp}")
              if m then
                value = m[1].gsub(/\s/, ' ')
                curXML.last.element.attrs[key] = value
                line = m.post_match
              else
                $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
              end
            end
          elsif $& == ">" then
            if $`[-1,1] == '/' then
              # end of empty (no pcdata) element...
              t = curXML.pop
              if curXML.empty? then
                self.child << t
                seeking = qLT
              else
                curXML.last.child << t
                seeking = qPCD
              end
            else
              seeking = qPCD
            end
            line = $'
          else
            $stderr.puts ">>>ERROR: couldn't find closing bracket of #{tag} element#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            done = TRUE
          end
          
        when qPCD
          line << (f.gets || "") until line =~ /<\s*(!\[CDATA\[)|<(!--)|<\s*([^>\s][^>\s\[\/]*)/ || f.eof
          if $1 || $2 || $3 then
            data = $` #.chomp  # pre-match
            tag = $1 || $2 || $3
            line = $'  # post-match
            # first save pcdata as new element with blank tag... 
            unless data.empty? || curXML.empty? then
              curXML.last.child << IXML.new
              curXML.last.child.last.element.pcdata = data
            end
            case tag 
              when /^\/(.*)/
                # check closing tag against top of curXML stack ...
                if tag != "/" + curXML.last.element.tag then
                  $stderr.puts ">>>ERROR: mis-matched begin/end tags #{curXML.last.element.tag}/#{tag}#{($. && $. > 0) ? " line:#{$..to_s}" : "" }" 
                  line.insert(0, "<" + tag)
                else
                  # find the end bracket of the closing tag...
                  line << (f.gets || "") until line =~ />/ || f.eof
                  line = $' || line
                end
                t = curXML.pop
                if curXML.empty? then
                  self.child << t
                  seeking = qLT
                else
                  curXML.last.child << t
                  seeking = qPCD
                end
              when /!\[CDATA\[/
                seeking = qCDATA
                finis = /\]\]>/
              when /script/
                seeking = qSCRIPT
                finis = /<\/script>|\/>/
              when /!DOCTYPE/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qDOCTYPE
              when /!ENTITY/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qENTITY
              when /!--/
                seeking = qCOM
                finis = /-->/
              when /!.+/
                seeking = qDECL
                finis = /\]>/
              when /\?.*/
                seeking = qPI
                finis = /\?>/
              else
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qATT
            end
          else
            if !curXML.empty? then
              $stderr.puts ">>>ERROR: couldn't find #{tag} element closing tag (readXML)#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            end
            done = TRUE
          end
          
        when qCDATA, qDECL, qPI
          # find the end of the section...
          line << (f.gets || "") until line =~ finis || f.eof
          line = $' || line
          seeking = qPCD
          if $` then
            # new sub-element...
            t = IXML.new(Element.new(tag, Hash.new, $`))
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of #{tag} section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
          
        when qSCRIPT
          # find the end of the section...
          line << (f.gets || "") until line =~ finis || f.eof
          line = $' || line
          seeking = qPCD
          if $` then
            temp = $`
            if temp =~ /([^>]*)>?/ then
              attrs = $1
              data = $'
              a = {}
              while attrs =~ /([^="'\s]+)\s*=\s*('|")([^'"]*)\2/ do
                a[String.new($1)] = String.new($3)
                attrs = $'
              end
              t = IXML.new(Element.new(tag, a, ""))
              u = IXML.new(Element.new("", {}, data))
              t.child << u
            else
              t = IXML.new(Element.new(tag, {}, ""))
            end
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of #{tag} section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
          
        when qCOM
          # find the end of the comment...
          line << (f.gets || "") until line =~ finis || f.eof
          line = $' || line
          seeking = qPCD
          if $` then
            # new sub-element...
            t = IXML.new(Element.new(tag, Hash.new, $`))
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of comment#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
          
          #if $&.empty? then $stderr.puts ">>>ERROR: couldn't find end of comment" end
          
        when qDOCTYPE
          line << (f.gets || "") until line =~ /\[|"|'|>/ || f.eof
          if $& == "'" || $& == '"' then
            data = $`  # pre-match
            line = $'  # post-match
            temp = $&  # match
            line << (f.gets || "") until f.eof || line.count(temp) > 0
            m = line.match("([^#{temp}]*?)#{temp}")
            if m then
              data << temp + m[1] + temp
              curXML.last.element.pcdata << data.gsub(/\s/, ' ')
              line = m.post_match
            else
              $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            end
          elsif $& == "[" then
            curXML.last.element.pcdata << $`.chomp  # pre-match
            line = $'  # post-match
            line << (f.gets || "") until f.eof || line =~ /\]\s*>/
            seeking = qPCD
            if !$&.empty? then
              # new sub-element...
              t = IXML.new(Element.new("[", Hash.new, ""))
              t.parseXML($`)
              line = $'
              curXML.last.child << t  # place sub-tree under DOCTYPE child
              t = curXML.pop
              if curXML.empty? then
                self.child << t
                seeking = qLT
              else
                curXML.last.child << t
              end
            else
              $stderr.puts ">>>ERROR: couldn't find end of internal DTD sub-set#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
            end
          elsif $& == ">" then
            curXML.last.element.pcdata << $`.chomp  # pre-match
            line = $'  # post-match
            t = curXML.pop
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
              seeking = qPCD
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of !DOCTYPE section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
            done = TRUE
          end
          
        when qENTITY
          line << (f.gets || "") until line =~ /"|'|>/ || f.eof
          if $& == "'" || $& == '"'then
            data = $`  # pre-match
            line = $'  # post-match
            temp = $&  # match
            line << (f.gets || "") until f.eof || line.count(temp) > 0
            m = line.match("([^#{temp}]*?)#{temp}")
            if m then
              data << temp + m[1] + temp
              curXML.last.element.pcdata << data.gsub(/\s/, ' ')
              line = m.post_match
            else
              $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            end
          elsif $& == ">" then
            curXML.last.element.pcdata << $`.chomp  # pre-match
            line = $'  # post-match
            t = curXML.pop
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
              seeking = qPCD
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of !ENTITY section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
            done = TRUE
          end
          
      end
    end until done
  end  # readXML()
  

  def parseXML(buffer)
    qLT = 0
    qATT = 1
    qPCD = 2
    qCDATA = 3
    qDOCTYPE = 4
    qCOM = 5
    qDECL = 6
    qPI = 7
    qENTITY = 8
    
    curXML = []
    seeking = qLT
    line = buffer
    done = FALSE
    begin
      case seeking 
        when qLT
          if line =~ /<\s*(!\[CDATA\[)|<(!--)|<\s*([^>\s][^>\s\[\/]*)/ then
            tag = $1 || $2 || $3
            line = $'  # post-match
            #~ $stderr.puts "tag[#{tag}],line[#{line}]"  # debug
            case tag 
              when /!\[CDATA\[/
                seeking = qCDATA
                finis = /\]\]>/
              when /script/
                seeking = qSCRIPT
                finis = /<\/script>|\/>/
              when /!DOCTYPE/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qDOCTYPE
              when /!ENTITY/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qENTITY
              when /!--/
                #~ $stderr.puts line  # debug
                seeking = qCOM
                finis = /-->/
              when /!.+/
                seeking = qDECL
                finis = /\]>/
              when /\?.*/
                seeking = qPI
                finis = /\?>/
              else
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qATT
            end
          else
            done = TRUE
          end
          
        when qATT
          if line =~ /=|>/ then
            if $& == "=" then
              key = $`.strip 
              line = $'  # post-match
              if line =~ /"|'/ then
                temp = $&
                m = line.match("#{temp}([^#{temp}]*?)#{temp}")
                if m then
                  value = m[1].gsub(/\s/, ' ')
                  curXML.last.element.attrs[key] = value
                  line = m.post_match
                else
                  # ERROR: no matching closing quote (" or ')...
                  $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
                end
              end
            else  # $& == ">"
              if $`[-1,1] == '/'
                # end of empty (no pcdata) element...
                t = curXML.pop
                if curXML.empty? then
                  self.child << t
                  seeking = qLT
                else
                  curXML.last.child << t
                  seeking = qPCD
                end
              else
                seeking = qPCD
              end
              line = $'
            end
          else
            $stderr.puts ">>>ERROR: couldn't find closing bracket of #{tag} element#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            done = TRUE
          end
          
        when qPCD
          if line =~ /<\s*(!\[CDATA\[)|<(!--)|<\s*([^>\s][^>\s\[\/]*)/ then
            data = $` #.chomp  # pre-match
            tag = $1 || $2 || $3
            line = $'  # post-match
            # first save pcdata as new element with blank tag... 
            unless data.empty? || curXML.empty?
              curXML.last.child << IXML.new
              curXML.last.child.last.element.pcdata = data
            end
            case tag 
              when /^\/(.*)/
                # check closing tag against top of curXML stack ...
                if tag != "/" + curXML.last.element.tag
                  $stderr.puts ">>>ERROR: mis-matched begin/end tags #{curXML.last.element.tag}/#{tag}#{($. && $. > 0) ? " line:#{$..to_s}" : "" }" 
                  line.insert(0, "<" + tag)
                else
                  # find the end bracket of the closing tag...
                  line = $' if line =~ />/
                end
                t = curXML.pop
                if curXML.empty? then
                  self.child << t
                  seeking = qLT
                else
                  curXML.last.child << t
                  seeking = qPCD
                end
              when /!\[CDATA\[/
                seeking = qCDATA
                finis = /\]\]>/
              when /script/
                seeking = qSCRIPT
                finis = /<\/script>|\/>/
              when /!DOCTYPE/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qDOCTYPE
              when /!ENTITY/
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qENTITY
              when /!--/
                seeking = qCOM
                finis = /-->/
              when /!.+/
                seeking = qDECL
                finis = /\]>/
              when /\?.*/
                seeking = qPI
                finis = /\?>/
              else
                # new sub-element...
                t = IXML.new(Element.new(tag))
                curXML << t
                seeking = qATT
            end
          else
            if !curXML.empty? then
              $stderr.puts ">>>ERROR: couldn't find #{tag} element closing tag (parseXML)#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
            end
            done = TRUE
          end
          
        when qCDATA, qDECL, qPI
          # find the end of the section...
          seeking = qPCD
          if line =~ finis then
            # new sub-element...
            t = IXML.new(Element.new(tag, Hash.new, $`))
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
            end
            line = $'
          else
            $stderr.puts ">>>ERROR: couldn't find end of #{tag} section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
          
        when qSCRIPT
          # find the end of the section...
          seeking = qPCD
          if line =~ finis then
            temp = $`
            if temp =~ /([^>]*)>?/ then
              attrs = $1
              data = $'
              a = {}
              while attrs =~ /([^="'\s]+)\s*=\s*('|")([^'"]*)\2/ do
                a[String.new($1)] = String.new($3)
                attrs = $'
              end
              t = IXML.new(Element.new(tag, a, ""))
              u = IXML.new(Element.new("", {}, data))
              t.child << u
            else
              t = IXML.new(Element.new(tag, {}, ""))
            end
            if curXML.empty? then
              self.child << t
              seeking = qLT
            else
              curXML.last.child << t
            end
            line = $'
          else
            $stderr.puts ">>>ERROR: couldn't find end of #{tag} section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
        
        when qCOM
          # find the end of the comment...
          if line =~ finis then
            line = $' 
          else
            $stderr.puts ">>>ERROR: couldn't find end of comment#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
          end
          seeking = qPCD
          
        when qDOCTYPE
          if line =~ /\[|"|'|>/ then
            if $& == "'" || $& == '"' then
              data = $`  # pre-match
              line = $'  # post-match
              temp = $&  # match
              m = line.match("([^#{temp}]*?)#{temp}")
              if m then
                data << temp + m[1] + temp
                curXML.last.element.pcdata << data.gsub(/\s/, ' ')
                line = m.post_match
              else
                $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
              end
            elsif $& == "[" then
              curXML.last.element.pcdata << $`.chomp  # pre-match
              line = $'  # post-match
              seeking = qPCD
              if !$&.empty? then
                # new sub-element...
                t = IXML.new(Element.new("[", Hash.new, ""))
                t.parseXML($`)
                line = $'
                curXML.last.child << t  # place sub-tree under DOCTYPE child
                t = curXML.pop
                if curXML.empty? then
                  self.child << t
                  seeking = qLT
                else
                  curXML.last.child << t
                end
              else
                $stderr.puts ">>>ERROR: couldn't find end of internal DTD sub-set#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
              end
            elsif $& == ">" then
              curXML.last.element.pcdata << $`.chomp  # pre-match
              line = $'  # post-match
              t = curXML.pop
              if curXML.empty? then
                self.child << t
                seeking = qLT
              else
                curXML.last.child << t
                seeking = qPCD
              end
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of !DOCTYPE section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
            done = TRUE
          end
          
        when qENTITY
          if line =~ /"|'|>/ then
            if $& == "'" || $& == '"' then
              data = $`  # pre-match
              line = $'  # post-match
              temp = $&  # match
              m = line.match("([^#{temp}]*?)#{temp}")
              if m then
                data << temp + m[1] + temp
                curXML.last.element.pcdata << data.gsub(/\s/, ' ')
                line = m.post_match
              else
                $stderr.puts ">>>ERROR: couldn't find closing quote(#{temp})#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"
              end
            elsif $& == ">" then
              curXML.last.element.pcdata << $`.chomp  # pre-match
              line = $'  # post-match
              t = curXML.pop
              if curXML.empty? then
                self.child << t
                seeking = qLT
              else
                curXML.last.child << t
                seeking = qPCD
              end
            end
          else
            $stderr.puts ">>>ERROR: couldn't find end of !ENTITY section#{($. && $. > 0) ? " line:#{$..to_s}" : "" }"              
            done = TRUE
          end
          
      end
    end until done
  end  # parseXML
  

  @@context = []
  
  def cTraverse(matchContext)
    @@context.push(@element.to_s)
    if @@context.join("\n") =~ matchContext then
      yield @element
    end
    @child.each {|c| c.cTraverse(matchContext) {|e| yield(e)} }
    @@context.pop
  end  # cTraverse()


  def xhtml2txt(strict = false)
    result = ""
    
    if @element.tag == "bloq" || @element.tag == "olist" || @element.tag == "ulist" then
      $list << @element.tag
    end
    @child.each {|c| result << c.xhtml2txt(strict) }
    
    result.gsub!(/\s+\[note:/, " [note:")
    
    case @element.tag
      
      when ""
        result << @element.pcdata.translate_entities.chomp
        
      when "cite", "sub", "sup", "u", "i", "b", "tt", "span", "ins", "del", "strike", "em", "strong", "kbd"
        result.gsub!(/\s+/, " ")
      
      when "pre", "code"
        # leave whitespace intact
        result << "\n"
      
      when "p", "P"
        result.gsub!(/\s+/, " ")
        prefix = case @element.attrs["class"]
          when "RegisterBits" then "Bits:\t"
          when "RegisterClock" then "Clock:\t"
          when "RegisterReset" then "Reset State:\t"
          when "RegisterType" then "Type:\t"
          when "RegisterSecurity" then "Security Treatment:\t"
          when "RegisterTrigger" then "Trigger:\t"
          when "RegisterSetClear" then "Set Clear:\t"
          when "RegisterReadWaitStates" then "Read Wait States:\t"
          when "RegCellHedDescription" then "Description"
          when "RegCellHedName" then "Name"
          when "warning" then "WARNING: "
          when "caution" then "CAUTION: "
          when "notice" then "NOTE: "
          when "figure-title" then "Figure #{($section_numbers.first).to_s}-#{($figure_number += 1).to_s}: "
          #when "reference" then "[#{($reference_number += 1).to_s}] "
          else
            ""
        end
        prefix = "" if result =~ Regexp.new("^" + prefix)
        result = prefix + result
        suffix = case @element.attrs["class"]
          when "RegisterBits" then "\n"
          when "RegisterClock" then "\n"
          when "RegisterHeadAddress" then (strict ? "\n" : "\t")
          when "RegisterHeadName" then "\n"
          when "RegisterReset" then "\n\n"
          when "RegisterType" then "\n"
          when "RegisterSecurity" then "\n"
          when "RegisterTrigger" then "\n"
          when "RegisterSetClear" then "\n"
          when "RegisterReadWaitStates" then "\n"
          when "RegCellHedDescription" then ""
          when "RegCellHedName" then ""
          when /Anchor.+/ then ""
          else
            "\n\n"
        end
        result << suffix
      
      when "li", "dd", "div", "br"
        result.gsub!(/\s+/, " ")
        result << "\n"
        result.sub!(/\n+$/, "\n")
      
      when /h(\d)/
        result.gsub!(/\s+/, " ")
        level = $1.to_i
        (0..(level-1)).each do |i|
          if $section_numbers[i] then
            $section_numbers[i] += 1 if level == i
          else
            $section_numbers[i] = 1 
          end
        end
        result = "#{$section_numbers.join(".")} " + result + "\n"
        if level < 2 then
          $table_number = 0
          $figure_number = 0
          $equation_number = 0
          $reference_number = 0
        end
      
      when "a"
        if strict && @element.attrs.has_key?("href") && @element.attrs["href"] !~ /javascript/ then
          result = "<<href '#{@element.attrs["href"]}'>>" + result.strip + "<</href>>"  
        else
          result = ""
        end
        
      when "dl"
        result << "\n\n"
        
      when "dt"
        result << ":\t"
        
      when "fn"
        result = " [note: " + result.strip + "] "
      
      when "script", "colgroup"
        result = ""
        
      when "ol"
        if @element.attrs.has_key?("continue") then
          $olist_number = @element.attrs["continue"].to_i
        else
          $olist_number = 0
        end
        result.gsub!(/\n\n(.)/) {|m| "\n#{$1}" }
        result.gsub!(/(^|\n)\t/) {|m| "#{$1}\t\t" }
        result.gsub!(/(^|\n)([^\t\n])|(^|\n)(\t+)((\d+)|([a-z])|([ivxl]+))\./m) do |m| 
          if $3 then
            indent = $4.length
            if $6 then
              n = $6.to_i
            elsif $7 then
              n = from_alphameric($7)
            elsif $8 then
              n = from_roman($8)
            end
            num = case (indent % 3)
              when 0 then to_roman(n)  # convert to roman numeral
              when 1 then n.to_s  # use decimal
              when 2 then to_alphameric(n)  # convert to alphameric
            end
            "#{$3}#{$4}#{num}."
          else
            "#{$1}\t#{($olist_number += 1).to_s}. #{$2}"
          end
        end
        result << "\n"
      
      when "ul"
        result.gsub!(/\n\n(.)/) {|m| "\n#{$1}" }
        result.gsub!(/(^|\n)\t/) {|m| "#{$1}\t\t" }
        result.gsub!(/(^|\n)([^\t\n])/) {|m| "#{$1}\to #{$2}" }
        result << "\n"
      
      when "blockquote"
        result.gsub!(/ +/, " ")
        result.gsub!(/([^\n\t])\t/) {|m| "#{$1}\n\t" }
        result.gsub!(/(^|\n)(.)/m) {|m| "#{$1}\t#{$2}" }
        result << "\n\n"
      
      when "img"
        result << " [figure '#{@element.attrs["src"]}'] "
      
      when "q"
        result = "\"" + result + "\"" unless result.empty?
      
      when "caption"
        result.gsub!(/\s+/, " ")
        result = "{caption}" + result.strip + "\n"
      
      when "table"
        result.sub!("{caption}",  "Table #{($table_number += 1).to_s}: ")
        if strict then
          result = "<<table>>\n" + result + "<</table>>\n\n"
        else
          result = result + "\n\n"
        end
        
      when "thead", "tbody", "tfoot", "tgroup"
        
      when "tr"
        if strict then
          result = result.gsub(/,$/, "") + "\n"
        else
          result = result.gsub(/\t$/, "") + "\n"
        end
        
      when "td", "th"
        #result.strip!
        if strict then
          result = result.rstrip.csv + ","
        else
          result = result.gsub(/\s+/, " ") + "\t"
        end
        
      else
        #result.gsub!(/^\n+|\n+$/, "")
        
    end
    if @element.tag == "bloq" || @element.tag == "olist" || @element.tag == "ulist" then
      $list.pop
    end
    result.gsub!(/\n{3,}/, "\n\n")
    return result
  end  # xhtml2txt
  
end  # IXML


$interleaf = %w(Q U A L C O M M C o n f i d e n t i a l a n d P r o p r i e t a r y)
$interleaf_len = 34
$il_string = "QUALCOMM&copy;"
$il_len = 9  # copyright entity should occupy just one character length

$before = "eoQUALcOMm30 twWNBiu.paKCSPEZvnrkDVg,l1sf5IRFdYx96y2XHbz7J84qhT"
$after = "¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"

def copyright(directory, interleave = true)
  paths = {}
  here = directory.gsub("\\", "/")
  here << "/" unless here[-1,1] == "/"
  
  if RUBY_PLATFORM =~ /(mswin|mingw)32/ then
    a = Dir.glob("#{here}**/*.{html}")
  else
    a = Dir.glob("#{here}**/*.{HTML,html}")
  end
    
  a.each do |fn|
    next if fn =~ /\.(navi|index(\.[A-Z])?|reglist)\.html$/
    #~ next if fn =~ /(cover|footer)\.html$/
    
    $stderr.print fn  # debug
    x = IXML.new
    File.open(fn, "r:UTF-8") {|input| x.readXML(input) }
    
    modify = false
    i = -1
    x.walkc do |y, context|
      #~ $stderr.puts "    y=#{y.element.tag}; context=[#{context.join(",")}]"  # debug
      if interleave && y.element.tag.empty? && !y.element.pcdata.chomp.empty? && !context.any? {|c| c =~ /^((code|msg)(block)?|pre|kbd|tt|var|head|thead|script)$/ } then
        y.element.pcdata.gsub!(/&([A-Za-z\d#]+);|\\\(.+?\\\)|[^\s&]+/) do |s|
          #~ s << "\uFEFF"  # !!! because unix ruby interpreter has a bug
          if s[0] == "&" || s[0..1] == "\\(" then
            s
          else
            modify = true
            s.tr!($before, $after)
            s.gsub!(/./) do |s2| 
              i += 1
              i = 0 if i >= $interleaf_len
              "<q>#{$interleaf[i]}</q>" +  s2
            end
            "<span class='Q'>#{s}</span>"
          end
        end
      elsif y.element.tag.empty? && !y.element.pcdata.chomp.empty? && !context.any? {|c| c =~ /^((code|msg)(block)?|kbd|tt|var|head|thead|script)$/ } then
        y.element.pcdata.gsub!(/&([A-Za-z\d#]+);|\\\(.+?\\\)|[^\s&]+/) do |s|
          #~ s << "\uFEFF"  # !!! because unix ruby interpreter has a bug
          if s[0] == "&" || s[0..1] == "\\(" then
            s
          else
            modify = true
            "#{s.tr($before, $after)}"
          end
        end
        y.element.pcdata.gsub!(" ", "  ")  # two spaces for one
      end
    end
    
    if interleave then
      x.walk do |y|
        n = (y.child.length - 1)
        while n >= 0 do
          if y.child[n].element.tag == "p" then
            temp = IXML.new(Element.new("p", {"class" => "Q2"}))
            pcd = IXML.new(Element.new("", {}, $il_string))
            temp.child << pcd
            y.child.insert(n, temp)
            #~ $stderr.print "!"  # debug
            n -= 1
          end
          n -= 1
        end
      end
    end
    
    if modify then
      File.open(fn, "w:UTF-8") {|output| x.writeXML_ws(output) } 
      $stderr.puts "*"
    else
      $stderr.puts
    end
  end
end  # copyright


# main...
if $0 == __FILE__ then
  copyright((ARGV[0] ? ARGV[0] : "."), false)
end

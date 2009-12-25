#!/usr/bin/python

# script to read .todo files (in the xml format of the 'devtodo' program)
# a quick-and-dirty replacement for devtodo for systems where it won't build
# correctly or isn't available (mac os x 10.6)

from xml.dom import minidom
import sys
import string
import time
import datetime
import optparse
import color

# TODO: 
# - pretty xml formatting: need to write new xml content with 'indent', 'addindent' and 'newl' parameters, while the existing xml needs to *not* have these (it is already formatted)...
# - command line switches:
#   - purge items older than 'n' days
#   - show items of priority 'n' or less (not too important, grep can get us
#   this)
#   - sort items (default: by priority) (also not that important, sort can do
#   this for us. e.g. 'todo.py|sort -t . -k 1' sorts by number instead of
#   the default sort-by-priority)



# Priorities: 
# veryhigh, high, medium, low, verylow
def pri2int( pri ):
    if pri == "veryhigh":
        return 1
    elif pri == "high":
        return 2
    elif pri == "medium":
        return 3
    elif pri == "low":
        return 4
    elif pri == "verylow":
        return 5

class Item( object ):
    # class variable for numbering items
    count = 0

    def __init__( self, xmlnode ):
        Item.count += 1
        self.num = Item.count
        self.children = []
        # ensure the name of the node is 'note' and that it has 'priority' and
        # 'time' attributes
        if xmlnode.nodeName != 'note':
            raise "Invalid node: not a note"
        if not xmlnode.hasAttribute( "priority" ):
            raise "Invalid node: no priority attribute";
        if not xmlnode.hasAttribute( "time" ):
            raise "Invalid node: no time attribute";
        # strip whitespace from title
        self.title = string.strip( xmlnode.firstChild.toxml() )
        self.priority = xmlnode.getAttribute( "priority" )
        self.time = int( xmlnode.getAttribute( "time" ) )
        # if it has a "done" attribute, get the time for that
        self.done = None
        if xmlnode.hasAttribute( "done" ):
            self.done = int( xmlnode.getAttribute( "done" ) )
        for child in xmlnode.childNodes:
            if child.nodeName == "comment":
                # strip whitespace from comment
                self.comment = string.strip( child.firstChild.toxml() )
            elif child.nodeName == "note":
                self.children.append( Item( child ) )

    def __str__( self ):
        return self.title
    
    def getTimeStr( self ):
        return time.ctime( self.time )

    def getColor( self ):
        if self.priority == "veryhigh":
            return "${RED}"
        elif self.priority == "high":
            return "${YELLOW}"
        elif self.priority == "medium":
            return "${GREEN}"
        elif self.priority == "low":
            return "${CYAN}"
        elif self.priority == "verylow":
            return "${BLUE}"

    def printItem( self, term, child = False, verbose = False, show_if_done = False ):
        if self.done:
            if not show_if_done:
                return
            if child:
                template = "\t-"
            else:
                template = "- "
        else:
            if child:
                template = "\t"
            else:
                template = "  "
        # if this is a tty, use color
        if sys.stdout.isatty():
            color = self.getColor()
            template += str( self.num ) + "." + "${BOLD}" + color + str( self ) + "${NORMAL}\n"
            text = term.render( template )
        # otherwise, no color
        else:
            text = template + str( self.num ) + "." + "[pri." + str( pri( self ) ) + "]" + str( self ) + "\n"
        sys.stdout.write( text )
        # dump children (with indent) & done status
        for c in self.children:
            c.printItem( term, child=True )

def pri( item ):
    return pri2int( item.priority )

def writeXml( xml ):
    """write the .todo file (in the pwd), using the list of items passed"""
    file = open( ".todo", "w" )
    xml.writexml( file )


if __name__ == "__main__":
    # parse the options
    op = optparse.OptionParser()
    op.add_option( "-A", "--All", action="store_true", dest="show_all", help="Show all items, even those that are already done." )
    op.add_option( "-d", "--done", dest="done", help="Mark item 'done'." )
    op.add_option( "-a", "--add", action="store_true", dest="add", help="Add new item." )
    op.add_option( "-p", "--parent", dest="parent", help="Add new item with this parent." )
    op.add_option( "-D", "--delete", dest="delete", help="Delete an item permanently." )
    op.add_option( "-P", "--purge", dest="purge", help="Delete all items older than 'n' days." )
    (options, args) = op.parse_args()

    # terminal handler
    term = color.TerminalController()

    # open the file (from the current working directory)
    try:
        todo_file = open( ".todo" )
    except:
        # file doesn't exist? prompt to create it
        print( "File .todo doesn't exist in this directory.\nCreate new .todo file? (y/n):" )
        text = sys.stdin.readline()
        if( len( text ) != 0 ):
            if( text[0] != 'y' ):
                exit( 0 )
            todo_file = open( ".todo", "w" )
            xmltxt = """<?xml version="1.0" ?> <todo version="0.1.20"> </todo>"""
            todo_file.write( xmltxt )
            todo_file.close()
            try:
                todo_file = open( ".todo" )
            except:
                # file *still* doesn't exist or cannot be opened? bail
                print( "error: unable to open .todo file." )
                exit( -1 )
        else:
            exit( 0 )

    # parse the XML
    todo_xml = minidom.parse( todo_file )

    # close the file
    todo_file.close()

    # first node is 'todo' node
    top = todo_xml.firstChild
    if top.nodeName != "todo":
        print( "error: invalid .todo file" )
        exit( -1 )

    # check version string
    #<todo version="0.1.20">
    ver = top.getAttribute( "version" )
    mmr = string.split( ver, "." )
    if( int( mmr[0] ) > 0 ):
        print( "error: .todo file version is newer than this program" )
        exit( -1 )
    elif( int( mmr[1] ) > 1 ):
        print( "error: .todo file version is newer than this program" )
        exit( -1 )
    elif( int( mmr[2] ) > 20 ):
        print( "error: .todo file version is newer than this program" )
        exit( -1 )

    # depending on the switches we were passed, display the list, add to the
    # list, mark an item done, ... 

    # mark an item complete
    if options.done:
        # read up the nth node
        nodes = todo_xml.getElementsByTagName( "note" )
        i = 1
        for node in nodes:
            if i == int( options.done ):
                print( "marking item #" + options.done + " 'done'" )
                node.setAttribute( "done", str( int( time.time() ) ) )
                break
            i += 1
        # prompt for a comment
        print( "comment:" )
        text = sys.stdin.readline()
        if( len( text ) != 0 ):
            txt_node = minidom.Text()
            txt_node.data = text
            element = todo_xml.createElement( "comment" )
            element.appendChild( txt_node )
            node.appendChild( element )

        # write out the xml
        writeXml( todo_xml )

    # delete an item permanently
    elif options.delete:
        # read up the nth node
        nodes = todo_xml.getElementsByTagName( "note" )
        i = 1
        for node in nodes:
            # call 'removeChild()' on the nth node
            if i == int( options.delete ):
                print( "deleting item #" + options.delete )
                parent = node.parentNode
                parent.removeChild( node )
                break
            i += 1
        # write out the xml
        writeXml( todo_xml )

    # purge items
    elif options.purge:
        days = int( options.purge )
        delta = datetime.timedelta( days=days )
        now = datetime.datetime.now()
        # read the nodes
        nodes = todo_xml.getElementsByTagName( "note" )
        i = 1
        for node in nodes:
            # call 'removeChild()' on any node completed longer than 'n' days ago
            if node.hasAttribute( "done" ):
                nodedt = datetime.datetime.fromtimestamp( int( node.getAttribute( "done" ) ) )
                diff = now - nodedt
                if diff >= delta:
                    print( "deleting item #" + str( i ) )
                    parent = node.parentNode
                    parent.removeChild( node )
            i += 1
        # write out the xml
        writeXml( todo_xml )
        
    # add a new item
    elif options.add:
        # get the priority for the new item
        print( "item priority (1 - 5):" )
        line = sys.stdin.readline()
        pri = int( line )
        if pri < 1 or pri > 5:
            print( "error: priority must be between 1 (veryhigh) and 5 (verylow)" )
            exit( -1 )
        if pri == 1: pri_text = "veryhigh"
        elif pri == 2: pri_text = "high"
        elif pri == 3: pri_text = "medium"
        elif pri == 4: pri_text = "low"
        elif pri == 5: pri_text = "verylow"

        # get the text for the new item
        print( "item text:" )
        text = sys.stdin.readline()
        txt_node = minidom.Text()
        txt_node.data = text

        # create the objects and add them to the DOM
        element = todo_xml.createElement( "note" )
        element.setAttribute( "time", str( int( time.time() ) ) )
        element.setAttribute( "priority", pri_text )
        element.appendChild( txt_node )

        # add the new element to the appropriate parent
        if options.parent:
            nodes = todo_xml.getElementsByTagName( "note" )
            i = 1
            for node in nodes:
                # if this is the parent node sought, add here
                if i == int( options.parent ):
                    node.appendChild( element )
                i += 1
        else:
            top.appendChild( element )

        # write out the xml
        writeXml( todo_xml )

        
    else:
        # walk the list of "note" nodes (possibly nested) under the top node,
        # building a list of items as we go
        notes = []
        for node in top.childNodes:
            if node.nodeType == top.ELEMENT_NODE and node.nodeName == "note":
                i = Item( node )
                notes.append( i )

        # no switches, just display the info
        notes.sort( key=pri )
        for note in notes:
#            if not options.show_all:
#                if not note.done:
#                    note.printItem( term )
#            else:
#                note.printItem( term )
            note.printItem( term, show_if_done = options.show_all )

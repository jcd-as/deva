#!/usr/bin/python

# script to read .todo files (in the xml format of the 'devtodo' program)
# a quick-and-dirty replacement for devtodo for systems where it won't build
# correctly or isn't available (mac os x 10.6)

from xml.dom import minidom
import sys
import string
import time
import optparse
import color

# TODO: 
# - command line switches:
#   purge items older than 'n' days
#   show items of priority 'n' or less
#   sort items (default: by priority)



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

    def printItem( self, term, child = False, verbose = False ):
#        print( str( self ) )
        color = self.getColor()
        if self.done:
            if child:
                template = "\t-"
            else:
                template = "- "
        else:
            if child:
                template = "\t"
            else:
                template = "  "
        template += str( self.num ) + "." + "${BOLD}" + color + str( self ) + "${NORMAL}\n"
        text = term.render( template )
        sys.stdout.write( text )
        # dump children (with indent) & done status
        for c in self.children:
#            print( "\t" + str( c ) )
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
    (options, args) = op.parse_args()

    # terminal handler
    term = color.TerminalController()

    # open the file (from the current working directory)
    todo_file = open( ".todo" )

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

    # TODO: depending on the switches we were passed, display the list, add to the
    # list, mark an item done...

    # delete an item
    if options.done:
        # read up the nth node
        nodes = todo_xml.getElementsByTagName( "note" )
        i = 1
        for node in nodes:
            # call 'removeChild()' on the nth node
            if i == int( options.done ):
                print( "deleting item #" + options.done )
                parent = node.parentNode
                parent.removeChild( node )
                break
            i += 1
        # write out the xml
        writeXml( todo_xml )

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
            if not options.show_all:
                if not note.done:
                    note.printItem( term )
            else:
                note.printItem( term )

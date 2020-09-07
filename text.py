class TextPiece:
    def __init__(self, buf, size, state, cap):
        self.buf = buf
        self.size = size
        self.state = state
        self.cap = cap

class TextTree:
    def __init__(self, root):
        self.root = root

class TextNode:
    def __init__(self, parent, left, lsize, right, rsize, piece):
        self.parent = parent
        self.left = left
        self.lsize = lsize
        self.right = right
        self.rsize = rsize
        self.piece = piece

def do_splay(t, cursor):
    n = TextNode(None, None, 0, None, 0, None)
    l = n
    r = n
    plo = 0

    if t is None:
        return

    while True:
        le = plo + t.lsize
        re = plo + t.lsize + t.piece.size
        if cursor <= le:
            if t.left is None:
                break
            if cursor <= plo + t.left.lsize:
                y = t.left
                t.left = y.right
                t.lsize = y.rsize
                if t.left:
                    t.left.parent = t
                y.right = t
                y.rsize += t.rsize + t.piece.size
                t.parent = y
                t = y
                if t.left is None:
                    break
            r.left = t
            r.lsize = t.lsize + t.rsize + t.piece.size
            if r != n:
                t.parent = r
            r = t
            t = t.left
        elif cursor > re:
            if t.right is None:
                break
            if cursor > re + t.right.lsize + t.right.piece.size:
                y = t.right
                t.right = y.left
                t.rsize = y.lsize
                if t.right is not None:
                    t.right.parent = t
                y.left = t
                y.lsize += t.lsize + t.piece.size
                t.parent = y
                t = y
                if t.right is None:
                    break
            l.right = t
            l.rsize = t.lsize + t.rsize + t.piece.size
            if l != n:
                t.parent = l
            l = t
            plo += t.lsize + t.piece.size
            t = t.right
        else:
            break

    l.right = t.left
    l.rsize = t.lsize
    r.left = t.right
    r.lsize = t.rsize

    if l.right is not None:
        l.right.parent = l
    if r.left is not None:
        r.left.parent = r

    lt = t.piece.size + t.lsize
    rt = t.piece.size + t.rsize

    y = n
    while y != r:
        y.lsize -= lt
        y = y.left
    y = n
    while y != l:
        y.rsize -= rt
        y = y.right

    t.left = n.right
    t.right = n.left
    t.lsize = t.left.lsize + t.left.rsize + t.left.piece.size
    t.rsize = t.right.lsize + t.right.rsize + t.right.piece.size

    t.parent = None
    return t

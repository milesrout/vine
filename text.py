import random
import time

step_by_step = False

class TextPiece:
    def __init__(self, buf, size, state, cap):
        self.buf = buf
        self.size = size
        self.state = state
        self.cap = cap

    def __str__(self):
        return self.buf + f'... (0x{self.size})'

class TextTree:
    def __init__(self, root):
        self.root = root

    def dot(self):
        with open('out.dot', 'w') as f:
            print('digraph vine {', file=f)
            print('        graph [ordering="out"];', file=f)
            print('        ratio = fill;', file=f)
            print('        forcelabels = true;', file=f)
            print('        node [style="filled"];', file=f)
            self.root.dot(f)
            print('}', file=f)

    def all_left_edges(self):
        return sorted(self.root.all_left_edges())

    def assert_is_good(self, ops):
        ops = ','.join(f'{x:x}' for x in ops)#map(str, ops))
        assert self.root.parent is None, ops
        self.root.assert_is_good(ops)

class TextNode:
    def __init__(self, parent, left, lsize, right, rsize, piece):
        self.parent = parent
        self.left = left
        self.lsize = lsize
        self.right = right
        self.rsize = rsize
        self.piece = piece

    def assert_is_good(self, ops):
        if self.parent:
            assert self is self.parent.left or self is self.parent.right, ops
        if self.left:
            assert self.left.parent is self, ops
            assert self.left.true_size == self.lsize, f'0x{self.left.true_size:x}:0x{self.lsize:x}:{ops}'
        if self.right:
            assert self.right.parent is self, ops
            assert self.right.true_size == self.rsize, f'0x{self.right.true_size:x}:0x{self.rsize:x}:{ops}'

    def parental_left_offset(self):
        if self.parent is None:
            return 0
        if self is self.parent.left:
            return self.parent.parental_left_offset()
        elif self is self.parent.right:
            return self.parent.right_edge()
        else:
            raise RuntimeError('Should not be possible!')

    def left_edge(self):
        return self.parental_left_offset() + self.lsize

    def right_edge(self):
        return self.parental_left_offset() + self.lsize + self.piece.size

    def do_size(self):
        if self.left is not None:
            self.left.do_size()
        if self.right is not None:
            self.right.do_size()
        self.lsize = self.left.size if self.left else 0
        self.rsize = self.right.size if self.right else 0

    @property
    def size(self):
        return self.lsize + self.piece.size + self.rsize

    @property
    def true_size(self):
        return (self.left.true_size if self.left else 0) + (self.right.true_size if self.right else 0) + self.piece.size

    def __str__(self):
        return '\n'.join(self.to_tree(0))

    def to_tree(self, indent):
        ind = indent * '\t'
        #tree = [f'{ind}{self.piece}']
        tree = [f'{ind}{hex(self.lsize + self.piece.size + self.rsize)}']
        if self.left is not None:
            tree.extend(self.left.to_tree(indent + 1))
        if self.right is not None:
            tree.extend(self.right.to_tree(indent + 1))
        return tree

    def dot(self, f):
        colour = 'crimson' if self.piece.state == 1 else 'cornflowerblue'
        lere = f'{self.left_edge():x}:{self.right_edge():x}'
        #lere = ''
        print(f'        "{id(self)}" [xlabel=<<font color="green">{lere}</font>>,label=<', file=f, end='')
        #print(f'        "{id(self)}" [label=<', file=f, end='')
        if self.left is None:
            print(f'{self.lsize:x}:', file=f, end='')
        if False:
            print(f'{self.piece}', file=f, end='')
        elif False:
            print(self.piece.size, file=f, end='')
        elif False:
            print(self.piece.size.bit_length() - 1, file=f, end='')
        elif False:
            print(f'2<sup>{self.piece.size.bit_length()-1}</sup>', file=f, end='')
        else:
            print(f'{self.piece.size:x} / 2<sup>{self.piece.size.bit_length()-1}</sup>', file=f, end='')
        if self.right is None:
            print(f':{self.rsize:x}', file=f, end='')
        print(f'>,color="{colour}"];', file=f)

        if self.left is not None:
            print(f'        "{id(self)}" -> "{id(self.left)}" [fontcolor="red",label="{self.lsize:x}"];', file=f)
            #print(f'        "{id(self)}" -> "{id(self.left)}";', file=f)
            self.left.dot(f)
        if self.right is not None:
            print(f'        "{id(self)}" -> "{id(self.right)}" [fontcolor="red",label="{self.rsize:x}"];', file=f)
            #print(f'        "{id(self)}" -> "{id(self.right)}";', file=f)
            self.right.dot(f)
        if self.parent is not None:
            print(f'        "{id(self)}" -> "{id(self.parent)}" [style="dotted"];', file=f)

    def all_left_edges(self):
        #ret = [random.randrange(self.left_edge(), self.right_edge())]
        ret = [self.left_edge()]
        if self.left is not None:
            ret.extend(self.left.all_left_edges())
        if self.right is not None:
            ret.extend(self.right.all_left_edges())
        return ret


def do_splay(t, cursor):
    n = TextNode(None, None, 0, None, 0, None)
    l = n
    r = n
    plo = 0

    l_or_r_first = None

    if t is None:
        return

    while True:
        #if step_by_step: breakpoint()
        le = plo + t.lsize
        re = plo + t.lsize + t.piece.size
        if cursor <= le and not (cursor == 0 and le == 0):
            if t.left is None:
                break
            if cursor <= plo + t.left.lsize:
                y = t.left
                t.left = y.right
                t.lsize = y.rsize
                if t.left is not None:
                    t.left.parent = t
                y.right = t
                y.rsize += t.rsize + t.piece.size
                t.parent = y
                t = y
                if t.left is None:
                    break
            if l_or_r_first is None:
                l_or_r_first = 'r'
            r.left = t
            r.lsize = t.size
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
            if l_or_r_first is None:
                l_or_r_first = 'l'
            l.right = t
            l.rsize = t.size
            if l != n:
                t.parent = l
            l = t
            plo += t.lsize + t.piece.size
            t = t.right
        else:
            break

    lt = r.lsize - t.rsize
    rt = l.rsize - t.lsize

    #breakpoint()

    l.right = t.left
    l.rsize = t.lsize
    r.left = t.right
    r.lsize = t.rsize

    if l.right is not None:
        l.right.parent = l
    if r.left is not None:
        r.left.parent = r

    #lt = t.piece.size + t.lsize
    #rt = t.piece.size + t.rsize

    y = n
    while y != r:
        #y.lsize -= lt
        y.lsize = y.left.true_size
        y = y.left
    y = n
    while y != l:
        y.rsize = y.right.true_size
        #y.rsize -= rt
        y = y.right

    t.left = n.right
    t.right = n.left
    if t.left is not None:
        t.lsize = t.left.size
    else:
        t.lsize = 0
    if t.right is not None:
        t.rsize = t.right.size
    else:
        t.rsize = 0

    if t.left is not None:
        t.left.parent = t
    if t.right is not None:
        t.right.parent = t

    t.parent = None
    return t

def generate_node(n):
    k = 2 ** (n - 1)
    p = TextPiece(chr(ord('A') + n - 1), k, 0, 0)
    return TextNode(None, None, 0, None, 0, p)

def generate_tree_k(k0, k):
    root = generate_node(k0)
    if k == 0:
        return root
    l = generate_tree_k(2 * k0, k - 1)
    r = generate_tree_k(2 * k0 + 1, k - 1)
    root.left = l
    root.lsize = l.size
    root.right = r
    root.rsize = r.size
    l.parent = root
    r.parent = root
    return root

def generate_tree(k):
    return TextTree(generate_tree_k(1, k))

def gen_node(n):
    k = n
    p = TextPiece(None, k, 0, 0)
    return TextNode(None, None, 0, None, 0, p)

def gen_tree_k(k0, k):
    root = gen_node(k0)
    if k == 0:
        return root
    l = gen_tree_k(k0 // (2 ** (2 ** (k - 1))), k - 1)
    r = gen_tree_k(k0 * (2 ** (2 ** (k - 1))), k - 1)
    root.left = l
    root.lsize = l.size
    root.right = r
    root.rsize = r.size
    l.parent = root
    r.parent = root
    return root

def gen_tree(k):
    return TextTree(gen_tree_k(2 ** (2 ** k - 1), k))


"""
1
   2
      8
         128
         256
      16
         512
         1024
   4
      32
         2048
         4096
      64
         8192
         16384
"""

def main():
    tr = generate_tree(3)
    #a = generate_node(1)
    #b = generate_node(2)
    #c = generate_node(3)
    #d = generate_node(4)
    #e = generate_node(5)
    #f = generate_node(6)
    #g = generate_node(7)
    #h = generate_node(8)
    #i = generate_node(9)
    #b.left = a; a.parent = b
    #c.left = b; b.parent = c
    #d.left = c; c.parent = d
    #e.left = d; d.parent = e
    #e.right = f; f.parent = e
    #f.right = g; g.parent = f
    #g.right = h; h.parent = g
    #h.right = i; i.parent = h
    #e.do_size()
    #tr = TextTree(e)
    tr.dot()
    while True:
        k = int(input('Enter number: '), 16)
        if k == -1:
            break
        print(', '.join('{:x}'.format(x) for x in tr.all_left_edges()))
        tr.root = do_splay(tr.root, k)
        tr.dot()

def test_all_trees(k):
    global step_by_step
    roots = set()
    for i in range(2**(k + 1) - 1):
        ops = []
        tr = gen_tree(k)
        tr.assert_is_good(ops)
        le = tr.all_left_edges()[i] + 1
        tr.assert_is_good(ops)
        #tr.dot()
        #input(str(le))
        tr.root = do_splay(tr.root, le)
        ops.append(le)
        tr.assert_is_good(ops)
        roots.add(tr.root.piece.size)
        tr.dot()

        #input()
        if False:
            les = tr.all_left_edges()
            random.shuffle(les)
            for le in les:
                tr.root = do_splay(tr.root, le)
                ops.append(le)
                tr.assert_is_good(ops)
                time.sleep(0.02)
                tr.dot()
                #input()
        else:
            tr.root = do_splay(tr.root, 0x7f)
            tr.dot()
            tr.assert_is_good(ops + [0x7f])
            tr.root = do_splay(tr.root, 0x7ff)
            tr.dot()
            #input()
            tr.assert_is_good(ops + [0x7f, 0x7ff])
            step_by_step = True
            tr.root = do_splay(tr.root, 0x1f)
            step_by_step = False
            tr.dot()
            #input()
            tr.assert_is_good(ops + [0x7f, 0x7ff, 0x1f])
    if len(roots) != 2**(k + 1) - 1:
        print('len(roots) wrong: expected {} but got {}'.format(2**(k + 1) - 1, len(roots)))

if __name__ == '__main__':
    while True:
        test_all_trees(int(input('k? ')))

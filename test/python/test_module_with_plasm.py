#!/usr/bin/env python
import ecto
import buster
from ecto import module 

class Mult(ecto.module):
    """Mult is documented"""
    def __init__(self, *args, **kwargs):
        print "HERE"
        ecto.module.__init__(self, **kwargs)
        print "THERE"
        
    @staticmethod
    def Params(params):
        params.set("factor", "multiply input by this", 4)
        
    def Config(self):
        self.inputs.set("input","mul", 2)
        self.outputs.set("out", "multed",8)
        
    def Process(self):
        a = self.params["factor"].val
        ### a = self.params.factor.val
        b = self.inputs.input
        self.outputs.out = b * a
        
class Compound(ecto.module):
    def __init__(self, *args, **kwargs):
        ecto.module.__init__(self, **kwargs)

    @staticmethod
    def Params(params):
        params.set("plasm", "subgraph", None)
        params.set("inputnames", "input names", None)
        params.set("otputnames", "outputs names", None)

    def Config(self):
        for i in self.params.inputnames:
            print "input", i
        for o in self.params.outputnames:
            print "output", o

    def Process(self):
        # get inputs from inputs
        # put them in to the plasm
        # plasm.go(last...

subplasm = make_some_plasm()

mod = Compound(plasm=subplasm, inputs=['foo_in', 'bar_in'], outputs=['foo_out', 'bar_out'])


def test_compound():
    gen = buster.Generate(start=2, step=3)
    print "OUTPUTS:::", gen.outputs.out
    assert gen.outputs.keys() == ['out']
    assert gen.outputs.out == -1
    print gen.inputs.keys()
    assert gen.inputs.keys() == []

    gen.Process()

    print gen.outputs.out
    assert gen.outputs.out == 2.0
    
    try:
        print gen.outputs.nonexistent
        assert "that should have thrown"
    except RuntimeError, e:
        assert str(e) == 'name does not exist!'

def test_my_module():
    t = ecto.tendrils()
    print t

    mm = MyModule(text="spam")
    mm.Config()
    mul = Mult(factor=4)
    mul.Config()
    printer = buster.Printer()
    printer.Config()
    print printer
    gen = buster.Generate(start=2, step=3)
    gen.Config()
    plasm = ecto.Plasm()
    print str(gen.outputs)
    print str(mul.inputs)
    plasm.connect(gen,"out",mul,"input")
    plasm.connect(mul,"out",mm, "input")
    plasm.connect(mm,"out",printer,"str")
    print plasm.go
    plasm.go(printer)
    plasm.mark_dirty(gen)
    plasm.go(printer)
    plasm.mark_dirty(gen)
    plasm.go(printer)
    ecto.print_module_doc(mul)
    ecto.view_plasm(plasm)
    print "it is:", mm.outputs['out'].val
    assert(mm.outputs["out"].val == "spamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspamspam")

if __name__ == '__main__':
    test_my_module()
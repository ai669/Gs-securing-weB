# Prover implementation for Weierstrass curves of the form
# y^2 = x^3 + A * x + B, specifically with a = 0 and b = 7, with group laws
# operating on affine and Jacobian coordinates, including the point at infinity
# represented by a 4th variable in coordinates.

load("group_prover.sage")


class affinepoint:
  def __init__(self, x, y, infinity=0):
    self.x = x
    self.y = y
    self.infinity = infinity
  def __str__(self):
    return "affinepoint(x=%s,y=%s,inf=%s)" % (self.x, self.y, self.infinity)


class jacobianpoint:
  def __init__(self, x, y, z, infinity=0):
    self.X = x
    self.Y = y
    self.Z = z
    self.Infinity = infinity
  def __str__(self):
    return "jacobianpoint(X=%s,Y=%s,Z=%s,inf=%s)" % (self.X, self.Y, self.Z, self.Infinity)


def point_at_infinity():
  return jacobianpoint(1, 1, 1, 1)


def negate(p):
  if p.__class__ == affinepoint:
    return affinepoint(p.x, -p.y)
  if p.__class__ == jacobianpoint:
    return jacobianpoint(p.X, -p.Y, p.Z)
  assert(False)


def on_weierstrass_curve(A, B, p):
  """Return a set of zero-expressions for an affine point to be on the curve"""
  return constraints(zero={p.x^3 + A*p.x + B - p.y^2: 'on_curve'})


def tangential_to_weierstrass_curve(A, B, p12, p3):
  """Return a set of zero-expressions for ((x12,y12),(x3,y3)) to be a line that is tangential to the curve at (x12,y12)"""
  return constraints(zero={
    (p12.y - p3.y) * (p12.y * 2) - (p12.x^2 * 3 + A) * (p12.x - p3.x): 'tangential_to_curve'
  })


def colinear(p1, p2, p3):
  """Return a set of zero-expressions for ((x1,y1),(x2,y2),(x3,y3)) to be collinear"""
  return constraints(zero={
    (p1.y - p2.y) * (p1.x - p3.x) - (p1.y - p3.y) * (p1.x - p2.x): 'colinear_1',
    (p2.y - p3.y) * (p2.x - p1.x) - (p2.y - p1.y) * (p2.x - p3.x): 'colinear_2',
    (p3.y - p1.y) * (p3.x - p2.x) - (p3.y - p2.y) * (p3.x - p1.x): 'colinear_3'
  })


def good_affine_point(p):
  return constraints(nonzero={p.x : 'nonzero_x', p.y : 'nonzero_y'})


def good_jacobian_point(p):
  return constraints(nonzero={p.X : 'nonzero_X', p.Y : 'nonzero_Y', p.Z^6 : 'nonzero_Z'})


def good_point(p):
  return constraints(nonzero={p.Z^6 : 'nonzero_X'})


def finite(p, *affine_fns):
  con = good_point(p) + constraints(zero={p.Infinity : 'finite_point'})
  if p.Z != 0:
    return con + reduce(lambda a, b: a + b, (f(affinepoint(p.X / p.Z^2, p.Y / p.Z^3)) for f in affine_fns), con)
  else:
    return con

def infinite(p):
  return constraints(nonzero={p.Infinity : 'infinite_point'})


def law_jacobian_weierstrass_add(A, B, pa, pb, pA, pB, pC):
  """Check whether the passed set of coordinates is a valid Jacobian add, given assumptions"""
  assumeLaw = (good_affine_point(pa) +
               good_affine_point(pb) +
               good_jacobian_point(pA) +
               good_jacobian_point(pB) +
               on_weierstrass_curve(A, B, pa) +
               on_weierstrass_curve(A, B, pb) +
               finite(pA) +
               finite(pB) +
               constraints(nonzero={pa.x - pb.x : 'different_x'}))
  require = (finite(pC, lambda pc: on_weierstrass_curve(A, B, pc) +
             colinear(pa, pb, negate(pc))))
  return (assumeLaw, require)


def law_jacobian_weierstrass_double(A, B, pa, pb, pA, pB, pC):
  """Check whether the passed set of coordinates is a valid Jacobian doubling, given assumptions"""
  assumeLaw = (good_affine_point(pa) +
               good_affine_point(pb) +
               good_jacobian_point(pA) +
               good_jacobian_point(pB) +
               on_weierstrass_curve(A, B, pa) +
               on_weierstrass_curve(A, B, pb) +
               finite(pA) +
               finite(pB) +
               constraints(zero={pa.x - pb.x : 'equal_x', pa.y - pb.y : 'equal_y'}))
  require = (finite(pC, lambda pc: on_weierstrass_curve(A, B, pc) +
             tangential_to_weierstrass_curve(A, B, pa, negate(pc))))
  return (assumeLaw, require)


def law_jacobian_weierstrass_add_opposites(A, B, pa, pb, pA, pB, pC):
  assumeLaw = (good_affine_point(pa) +
               good_affine_point(pb) +
               good_jacobian_point(pA) +
               good_jacobian_point(pB) +
               on_weierstrass_curve(A, B, pa) +
               on_weierstrass_curve(A, B, pb) +
               finite(pA) +
               finite(pB) +
               constraints

-- set - a container for unordered unique elements.
-- @module set
-- @alias Set

-- modified from lua stdlib: https://github.com/lua-stdlib

local Set = {}
Set.__index = Set

function Set.new(elements)
  local o = setmetatable({}, Set)
  o._e = {}
  if elements ~= nil then
    for _, e in ipairs(elements) do
      o:insert(e)
    end
  end
  return o
end

function Set:insert(e)
  return rawset(self._e, e, true)
end

function Set:remove(e)
  table.remove(self._e, e)
end

function Set:contains(e)
  return rawget(self._e, e) == true
end

function Set:union(other)
  local s = Set.new(self:to_array())
  for e in next, other._e do
    s:insert(e)
  end
  return s
end

function Set:intersect(other)
  local s = Set.new({})
  for e in next, self._e do
    if other:contains(e) then
      s:insert(e)
    end
  end
  return s
end

function Set:difference(other)
  local s = Set.new({})
  for e in next, self._e do
    if not other:contains(e) then
      s:insert(e)
    end
  end
  return dif
end

function Set:symmetric_difference(other)
  s1 = self:union(other)
  s2 = other:intersect(self)
  return s1:difference(s2)
end

function Set:is_subset_of(other)
  for e in next, self._e do
    if not other:contains(e) then
      return false
    end
  end
  return true
end

function Set:is_propper_subset_of(other)
  return self:is_subset_of(other) and not other:is_subset_of(self)
end

function Set:__eq(other)
  return self:is_subset_of(other) and other:is_subset_of(self)
end

function Set:count()
  return rawlen(self._e)
end

function Set:to_array()
  local a = {}
  for e in next, self._e do
    table.insert(t, e)
  end
  return a
end

return Set


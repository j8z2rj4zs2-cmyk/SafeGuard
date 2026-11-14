-- =====================================================================
-- SafeGuard v2 — offset-aware build
-- offsets: theo (version-4ea8e85481e34f46)
-- toggle: [9]
-- =====================================================================

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local UIS = game:GetService("UserInputService")
local TweenService = game:GetService("TweenService")
local CoreGui = game:GetService("CoreGui")
local Workspace = game:GetService("Workspace")
local RepStorage = game:GetService("ReplicatedStorage")
local VirtualUser = game:GetService("VirtualUser")

local lp = Players.LocalPlayer
local function char() return lp.Character end
local function root() local c = char(); return c and c:FindFirstChild("HumanoidRootPart") end
local function hum()  local c = char(); return c and c:FindFirstChildOfClass("Humanoid") end
local function cam()  return Workspace.CurrentCamera end

pcall(function() if _G.SG and _G.SG.unload then _G.SG.unload() end end)

local SG = {}
_G.SG = SG

-- =====================================================================
-- OFFSETS — theo's dump (version-4ea8e85481e34f46)
-- =====================================================================
local OFF = {
    Instance = { Name = 0x0, ClassName = 0x8, Parent = 0x60, ChildrenStart = 0x70, ChildrenEnd = 0x8 },
    BasePart = { Primitive = 0x178, Transparency = 0x118, Color3 = 0x11c, CastShadow = 0x120 },
    Primitive = {
        Rotation = 0xb0, Position = 0xd4,
        AssemblyLinearVelocity = 0xe0, AssemblyAngularVelocity = 0xec,
        Flags = 0x1be, Size = 0x1c4, Owner = 0x218, Material = 0x24e,
    },
    Humanoid = {
        DisplayName = 0xa0, CameraOffset = 0x110, MoveDirection = 0x134, TargetPoint = 0x140,
        DisplayDistanceType = 0x174, FloorMaterial = 0x178, Health = 0x184, MaxHealth = 0x1a0,
        HipHeight = 0x188, JumpHeight = 0x198, JumpPower = 0x19c, MaxSlopeAngle = 0x1a4,
        NameDisplayDistance = 0x1a8, RigType = 0x1b8, Walkspeed = 0x1c8,
        AutoJumpEnabled = 0x1cc, AutoRotate = 0x1cd, PlatformStand = 0x1d6,
        Sit = 0x1d7, UseJumpPower = 0x1da, Jump = 0x1d4,
        HumanoidRootPart = 0x468, HumanoidState = 0x8b0, WalkspeedCheck = 0x3a4,
    },
    Camera = {
        Rotation = 0xc0, Position = 0xe4, CameraSubject = 0xb0, CameraType = 0x120,
        FieldOfView = 0x128, Viewport = 0x284, ViewportSize = 0x2c4, ImagePlaneDepth = 0x2d4,
    },
    Player = {
        LocalPlayer = 0x118, DisplayName = 0x120, ModelInstance = 0x2a0, Team = 0x2e0,
        AccountAge = 0x364, UserId = 0xb8,
    },
    VisualEngine = { Pointer = 0x8a11808, ViewMatrix = 0x1b0, Dimensions = 0xb10, FakeDataModel = 0xaf0, RenderView = 0xc50 },
    FakeDataModel = { Pointer = 0x8f8e968, RealDataModel = 0x1f8 },
    Workspace = { CurrentCamera = 0x4a0, DistributedGameTime = 0x4c0, World = 0x3f8 },
    PrimFlags = { Anchored = 0x2, CanCollide = 0x8, CanQuery = 0x20, CanTouch = 0x10 },
}

-- =====================================================================
-- MEMORY LAYER — uses executor's memory primitives when available
-- =====================================================================
local hasMem = type(memory_read) == "function" and type(memory_write) == "function"
local function instAddr(inst)
    local ok, addr = pcall(function() return inst.Address end)
    return (ok and type(addr) == "number") and addr or nil
end

local mem = {
    available = hasMem,
    rf  = function(a, o) if not hasMem or not a then return nil end; local ok, v = pcall(memory_read, "float", a + o); return ok and v or nil end,
    ri  = function(a, o) if not hasMem or not a then return nil end; local ok, v = pcall(memory_read, "int", a + o); return ok and v or nil end,
    rp  = function(a, o) if not hasMem or not a then return nil end; local ok, v = pcall(memory_read, "uintptr_t", a + o); return ok and v or nil end,
    rb  = function(a, o) if not hasMem or not a then return nil end; local ok, v = pcall(memory_read, "bool", a + o); return ok and v or nil end,
    wf  = function(a, o, v) if hasMem and a then pcall(memory_write, "float", a + o, v) end end,
    wb  = function(a, o, v) if hasMem and a then pcall(memory_write, "bool", a + o, v) end end,
    rvec3 = function(a, o)
        if not hasMem or not a then return nil end
        local x = mem.rf(a, o); local y = mem.rf(a, o + 4); local z = mem.rf(a, o + 8)
        if x and y and z then return Vector3.new(x, y, z) end
        return nil
    end,
    wvec3 = function(a, o, v)
        if not hasMem or not a or not v then return end
        mem.wf(a, o, v.X); mem.wf(a, o + 4, v.Y); mem.wf(a, o + 8, v.Z)
    end,
    rmat4 = function(a, o)
        if not hasMem or not a then return nil end
        local m = {}
        for i = 0, 15 do m[i + 1] = mem.rf(a, o + i * 4); if not m[i + 1] then return nil end end
        return m
    end,
    primOf = function(part)
        local a = instAddr(part); if not a then return nil end
        return mem.rp(a, OFF.BasePart.Primitive)
    end,
}

-- =====================================================================
-- STATE
-- =====================================================================
SG.enabled = {
    silentAim = false, aimlock = false, magicBullet = false,
    espBox = false, espHealth = false, espName = false, espSkeleton = false, espDistance = false,
    buildAutoPlace = false, buildTurbo = false, buildInstantEdit = false,
    desync = false, antiAfk = true, memBoost = false,
}
SG.config = {
    silentAim = { targetPart = "HumanoidRootPart", visibleCheck = false, hitChance = 100 },
    aimlock = { targetPart = "Head", smoothX = 4, smoothY = 4, sensitivity = 1, fov = 180, hold = true, key = Enum.UserInputType.MouseButton2 },
    magicBullet = { throughWalls = true, fovPx = 10 },
    esp = { maxDistance = 2000, boxColor = Color3.fromRGB(0, 255, 128), nameColor = Color3.fromRGB(255,255,255), skelColor = Color3.fromRGB(255,255,255) },
    desync = { mode = "jitter", amount = 15 },
    antiAfk = { interval = 60 },
    memBoost = { walkSpeed = 50, jumpPower = 100 },
}
local conns, running = {}, true
local function track(c) conns[#conns + 1] = c; return c end

SG.unload = function()
    running = false
    for _, c in ipairs(conns) do pcall(function() c:Disconnect() end) end
    conns = {}
    for k in pairs(SG.enabled) do SG.enabled[k] = false end
    pcall(function() if SG._ui then SG._ui:Destroy() end end)
    pcall(function() for pl, e in pairs(SG._esp or {}) do for _, d in pairs(e) do if typeof(d) == "table" then for _, dd in ipairs(d) do pcall(function() dd:Remove() end) end else pcall(function() d:Remove() end) end end end end)
    print("[SG] unloaded")
end

-- =====================================================================
-- OFFSET-AWARE WORLDTOSCREEN (uses ViewMatrix from VisualEngine if hasMem)
-- =====================================================================
local function readViewMatrix()
    if not hasMem then return nil end
    local ok, mat = pcall(function()
        local vePtr = mem.rp(0, OFF.VisualEngine.Pointer)
        if not vePtr then return nil end
        return mem.rmat4(vePtr, OFF.VisualEngine.ViewMatrix)
    end)
    return (ok and mat) or nil
end

local function worldToScreenMem(pos)
    local vm = readViewMatrix()
    if not vm then return nil, false end
    local vp = cam() and cam().ViewportSize or Vector2.new(1920, 1080)
    local cx = pos.X * vm[1] + pos.Y * vm[5] + pos.Z * vm[9]  + vm[13]
    local cy = pos.X * vm[2] + pos.Y * vm[6] + pos.Z * vm[10] + vm[14]
    local cw = pos.X * vm[4] + pos.Y * vm[8] + pos.Z * vm[12] + vm[16]
    if cw < 0.1 then return nil, false end
    local ndcX, ndcY = cx / cw, cy / cw
    return Vector2.new((ndcX * 0.5 + 0.5) * vp.X, (1 - (ndcY * 0.5 + 0.5)) * vp.Y), true
end

local function worldToScreen(pos)
    local v, on = worldToScreenMem(pos)
    if v and on then return v, true end
    -- fallback to Camera API
    local c = cam(); if not c then return Vector2.new(0, 0), false end
    local p, ok = c:WorldToViewportPoint(pos)
    return Vector2.new(p.X, p.Y), ok and p.Z > 0
end

-- =====================================================================
-- shared helpers
-- =====================================================================
local function enemies()
    local out = {}
    local myTeam = lp.Team
    for _, pl in ipairs(Players:GetPlayers()) do
        if pl ~= lp and pl.Character then
            local h = pl.Character:FindFirstChildOfClass("Humanoid")
            local hp = 100
            if hasMem and h then local a = instAddr(h); local v = a and mem.rf(a, OFF.Humanoid.Health); if v then hp = v end
            elseif h then hp = h.Health end
            if hp > 0 then
                if not myTeam or pl.Team ~= myTeam then
                    out[#out + 1] = pl
                end
            end
        end
    end
    return out
end

local function targetPartOf(plr, name)
    if not plr or not plr.Character then return nil end
    return plr.Character:FindFirstChild(name) or plr.Character:FindFirstChild("HumanoidRootPart")
end

local function partPosition(part)
    if not part then return nil end
    if hasMem then
        local prim = mem.primOf(part)
        if prim then local p = mem.rvec3(prim, OFF.Primitive.Position); if p then return p end end
    end
    return part.Position
end

local function isVisible(from, to)
    local rp = RaycastParams.new()
    rp.FilterDescendantsInstances = { lp.Character }
    rp.FilterType = Enum.RaycastFilterType.Exclude
    local res = Workspace:Raycast(from, to - from, rp)
    return res == nil
end

-- =====================================================================
-- MODULE 1: SILENT AIM via __namecall Vector3 rewrite
-- =====================================================================
local mt = pcall(getrawmetatable, game) and getrawmetatable(game) or nil
local oldNamecall
if mt and type(hookmetamethod) == "function" then
    oldNamecall = hookmetamethod(game, "__namecall", function(self, ...)
        if SG.enabled.silentAim then
            local method = getnamecallmethod and getnamecallmethod() or ""
            if (method == "FireServer" or method == "InvokeServer")
               and (self.ClassName == "RemoteEvent" or self.ClassName == "RemoteFunction") then
                local args = { ... }
                local target = nil
                local camCF = cam() and cam().CFrame
                if camCF then
                    local bestDot = 0.5
                    for _, pl in ipairs(enemies()) do
                        local p = targetPartOf(pl, SG.config.silentAim.targetPart)
                        if p then
                            local pos = partPosition(p)
                            if pos then
                                local to = (pos - camCF.Position)
                                if to.Magnitude > 0.1 then
                                    local d = camCF.LookVector:Dot(to.Unit)
                                    if d > bestDot then bestDot = d; target = p end
                                end
                            end
                        end
                    end
                end
                if target then
                    if SG.config.silentAim.hitChance < 100 and math.random(1,100) > SG.config.silentAim.hitChance then
                        return oldNamecall(self, ...)
                    end
                    local tp = partPosition(target)
                    if SG.config.silentAim.visibleCheck and camCF and tp and not isVisible(camCF.Position, tp) then
                        return oldNamecall(self, ...)
                    end
                    if tp then
                        for i, v in ipairs(args) do
                            if typeof(v) == "Vector3" then args[i] = tp
                            elseif typeof(v) == "CFrame" then args[i] = CFrame.new(tp) end
                        end
                    end
                    return oldNamecall(self, unpack(args))
                end
            end
        end
        return oldNamecall(self, ...)
    end)
end

-- =====================================================================
-- MODULE 2: AIMLOCK (offset-aware: writes Camera.Rotation when hasMem)
-- =====================================================================
track(RunService.RenderStepped:Connect(function()
    if not SG.enabled.aimlock then return end
    if SG.config.aimlock.hold then
        local held = false
        if SG.config.aimlock.key == Enum.UserInputType.MouseButton2 then held = UIS:IsMouseButtonPressed(Enum.UserInputType.MouseButton2)
        elseif SG.config.aimlock.key == Enum.UserInputType.MouseButton1 then held = UIS:IsMouseButtonPressed(Enum.UserInputType.MouseButton1) end
        if not held then return end
    end
    local c = cam(); if not c then return end
    local camCF = c.CFrame
    local best, bestAngle = nil, SG.config.aimlock.fov
    local vs = c.ViewportSize
    local crossX, crossY = vs.X * 0.5, vs.Y * 0.5
    for _, pl in ipairs(enemies()) do
        local p = targetPartOf(pl, SG.config.aimlock.targetPart)
        if p then
            local pos = partPosition(p)
            if pos then
                local screen, on = worldToScreen(pos)
                if on then
                    local dx, dy = screen.X - crossX, screen.Y - crossY
                    local ang = math.sqrt(dx*dx + dy*dy)
                    if ang < bestAngle then bestAngle = ang; best = p end
                end
            end
        end
    end
    if not best then return end
    local targetPos = partPosition(best); if not targetPos then return end
    local look = CFrame.new(camCF.Position, targetPos)
    local newPitch, newYaw = look:ToEulerAnglesYXZ()
    local curPitch, curYaw = camCF:ToEulerAnglesYXZ()
    local p = curPitch + (newPitch - curPitch) / math.max(SG.config.aimlock.smoothY, 0.1) * SG.config.aimlock.sensitivity
    local y = curYaw   + (newYaw   - curYaw)   / math.max(SG.config.aimlock.smoothX, 0.1) * SG.config.aimlock.sensitivity
    c.CFrame = CFrame.new(camCF.Position) * CFrame.fromEulerAnglesYXZ(p, y, 0)
end))

-- =====================================================================
-- MODULE 3: MAGIC BULLET
-- =====================================================================
track(UIS.InputBegan:Connect(function(input, gp)
    if gp or not SG.enabled.magicBullet then return end
    if input.UserInputType ~= Enum.UserInputType.MouseButton1 then return end
    if SG.enabled.silentAim then return end
    local wasSilent = SG.enabled.silentAim
    local wasVC = SG.config.silentAim.visibleCheck
    SG.enabled.silentAim = true
    SG.config.silentAim.visibleCheck = not SG.config.magicBullet.throughWalls
    task.delay(0.08, function()
        SG.enabled.silentAim = wasSilent
        SG.config.silentAim.visibleCheck = wasVC
    end)
end))

-- =====================================================================
-- MODULE 4: ESP (offset-aware position reads)
-- =====================================================================
local hasDrawing = type(Drawing) == "table" or type(Drawing) == "userdata"
local BONE_PAIRS = {
    {"Head","UpperTorso"}, {"UpperTorso","LowerTorso"},
    {"UpperTorso","LeftUpperArm"}, {"LeftUpperArm","LeftLowerArm"}, {"LeftLowerArm","LeftHand"},
    {"UpperTorso","RightUpperArm"}, {"RightUpperArm","RightLowerArm"}, {"RightLowerArm","RightHand"},
    {"LowerTorso","LeftUpperLeg"}, {"LeftUpperLeg","LeftLowerLeg"}, {"LeftLowerLeg","LeftFoot"},
    {"LowerTorso","RightUpperLeg"}, {"RightUpperLeg","RightLowerLeg"}, {"RightLowerLeg","RightFoot"},
}

SG._esp = {}
local function newD(kind) if not hasDrawing then return nil end; local ok, d = pcall(Drawing.new, kind); return ok and d or nil end
local function ensureEsp(pl)
    if SG._esp[pl] then return SG._esp[pl] end
    local e = {}
    e.box = newD("Square"); if e.box then e.box.Thickness=1.5; e.box.Filled=false; e.box.Visible=false end
    e.bar = newD("Line"); if e.bar then e.bar.Thickness=3; e.bar.Visible=false end
    e.barBg = newD("Line"); if e.barBg then e.barBg.Thickness=3; e.barBg.Color=Color3.new(0,0,0); e.barBg.Visible=false end
    e.name = newD("Text"); if e.name then e.name.Size=14; e.name.Center=true; e.name.Outline=true; e.name.Visible=false end
    e.dist = newD("Text"); if e.dist then e.dist.Size=12; e.dist.Center=true; e.dist.Outline=true; e.dist.Visible=false end
    e.bones = {}
    for i = 1, #BONE_PAIRS do local l = newD("Line"); if l then l.Thickness=1.2; l.Visible=false end; e.bones[i]=l end
    SG._esp[pl] = e
    return e
end
local function hideEsp(e)
    for _, k in ipairs({"box","bar","barBg","name","dist"}) do if e[k] then pcall(function() e[k].Visible=false end) end end
    if e.bones then for _, l in ipairs(e.bones) do if l then pcall(function() l.Visible=false end) end end end
end
track(Players.PlayerRemoving:Connect(function(pl) local e=SG._esp[pl]; if e then for _, k in ipairs({"box","bar","barBg","name","dist"}) do if e[k] then pcall(function() e[k]:Remove() end) end end; if e.bones then for _, l in ipairs(e.bones) do if l then pcall(function() l:Remove() end) end end end; SG._esp[pl]=nil end end))

track(RunService.RenderStepped:Connect(function()
    if not running then return end
    local anyOn = SG.enabled.espBox or SG.enabled.espHealth or SG.enabled.espName or SG.enabled.espSkeleton or SG.enabled.espDistance
    if not anyOn then for pl in pairs(SG._esp) do hideEsp(SG._esp[pl]) end; return end
    local myRoot = root()
    for _, pl in ipairs(Players:GetPlayers()) do
        if pl ~= lp and pl.Character then
            local e = ensureEsp(pl)
            local h = pl.Character:FindFirstChildOfClass("Humanoid")
            local hrp = pl.Character:FindFirstChild("HumanoidRootPart")
            local hp, maxHp = 100, 100
            if hasMem and h then
                local a = instAddr(h)
                if a then hp = mem.rf(a, OFF.Humanoid.Health) or 0; maxHp = mem.rf(a, OFF.Humanoid.MaxHealth) or 100 end
            elseif h then hp = h.Health; maxHp = h.MaxHealth end
            if h and hrp and hp > 0 then
                local hrpPos = partPosition(hrp)
                local head = pl.Character:FindFirstChild("Head") or hrp
                local foot = pl.Character:FindFirstChild("LeftFoot") or pl.Character:FindFirstChild("RightFoot") or hrp
                local headPos = partPosition(head) + Vector3.new(0, 0.6, 0)
                local footPos = partPosition(foot) - Vector3.new(0, 0.6, 0)
                local headS, hon = worldToScreen(headPos)
                local footS, fon = worldToScreen(footPos)
                local dist = myRoot and (hrpPos - myRoot.Position).Magnitude or 0
                if hon and fon and dist <= SG.config.esp.maxDistance then
                    local height = math.abs(footS.Y - headS.Y)
                    local width = height * 0.5
                    local boxX = (headS.X + footS.X) * 0.5 - width * 0.5
                    local boxY = headS.Y
                    if e.box then e.box.Visible=SG.enabled.espBox; e.box.Size=Vector2.new(width, height); e.box.Position=Vector2.new(boxX, boxY); e.box.Color=SG.config.esp.boxColor end
                    if e.bar and e.barBg then
                        local on = SG.enabled.espHealth
                        e.bar.Visible=on; e.barBg.Visible=on
                        if on then
                            local hpT = math.clamp(hp / math.max(maxHp, 1), 0, 1)
                            e.barBg.From = Vector2.new(boxX - 6, boxY); e.barBg.To = Vector2.new(boxX - 6, boxY + height)
                            e.bar.From = Vector2.new(boxX - 6, boxY + height * (1 - hpT)); e.bar.To = Vector2.new(boxX - 6, boxY + height)
                            e.bar.Color = Color3.fromRGB(math.floor(255*(1-hpT)), math.floor(255*hpT), 0)
                        end
                    end
                    if e.name then
                        e.name.Visible = SG.enabled.espName
                        e.name.Position = Vector2.new(boxX + width * 0.5, boxY - 16)
                        e.name.Text = pl.DisplayName or pl.Name
                        e.name.Color = SG.config.esp.nameColor
                    end
                    if e.dist then
                        e.dist.Visible = SG.enabled.espDistance
                        e.dist.Position = Vector2.new(boxX + width * 0.5, boxY + height + 2)
                        e.dist.Text = string.format("%d st", math.floor(dist))
                        e.dist.Color = SG.config.esp.nameColor
                    end
                    if SG.enabled.espSkeleton and e.bones then
                        for i, pair in ipairs(BONE_PAIRS) do
                            local a = pl.Character:FindFirstChild(pair[1])
                            local b = pl.Character:FindFirstChild(pair[2])
                            local l = e.bones[i]
                            if a and b and l then
                                local sa, onA = worldToScreen(partPosition(a))
                                local sb, onB = worldToScreen(partPosition(b))
                                if onA and onB then l.From=sa; l.To=sb; l.Color=SG.config.esp.skelColor; l.Visible=true
                                else l.Visible=false end
                            elseif l then l.Visible=false end
                        end
                    else
                        if e.bones then for _, l in ipairs(e.bones) do if l then l.Visible=false end end end
                    end
                else hideEsp(e) end
            else hideEsp(e) end
        else
            local e = SG._esp[pl]; if e then hideEsp(e) end
        end
    end
end))

-- =====================================================================
-- MODULE 5: BUILDING TOOLS
-- =====================================================================
task.spawn(function()
    while running do
        task.wait(0.03)
        if SG.enabled.buildAutoPlace then
            for _, p in ipairs(Workspace:GetDescendants()) do
                if p:IsA("ProximityPrompt") and p.Enabled then
                    local n = string.lower((p.ObjectText or "") .. " " .. (p.ActionText or ""))
                    if string.find(n, "build") or string.find(n, "place") or string.find(n, "wall") or string.find(n, "ramp") or string.find(n, "floor") then
                        pcall(fireproximityprompt, p)
                    end
                end
            end
        end
        if SG.enabled.buildTurbo then pcall(function() VirtualUser:ClickButton1(Vector2.new()) end) end
    end
end)

-- =====================================================================
-- MODULE 6: DESYNC (offset-aware position writes)
-- =====================================================================
task.spawn(function()
    while running do
        task.wait(0.05)
        if not SG.enabled.desync then continue end
        local r = root(); if not r then continue end
        local amt = SG.config.desync.amount
        if SG.config.desync.mode == "jitter" then
            local offset = Vector3.new((math.random()-0.5)*amt, (math.random()-0.5)*amt*0.2, (math.random()-0.5)*amt)
            if hasMem then
                local prim = mem.primOf(r)
                if prim then
                    local cur = mem.rvec3(prim, OFF.Primitive.Position)
                    if cur then mem.wvec3(prim, OFF.Primitive.Position, cur + offset) end
                end
            else
                r.CFrame = r.CFrame + offset
            end
        elseif SG.config.desync.mode == "lag" then
            r.AssemblyLinearVelocity = Vector3.new(0, r.AssemblyLinearVelocity.Y, 0)
        elseif SG.config.desync.mode == "fake-lag" then
            local orig = r.CFrame
            r.CFrame = orig * CFrame.new(amt, 0, 0)
            task.wait(0.03)
            if r.Parent then r.CFrame = orig end
        end
    end
end)

-- =====================================================================
-- MODULE 7: ANTI-AFK
-- =====================================================================
track(lp.Idled:Connect(function()
    if not SG.enabled.antiAfk then return end
    pcall(function() VirtualUser:CaptureController() end)
    pcall(function() VirtualUser:ClickButton2(Vector2.new()) end)
end))
task.spawn(function()
    while running do
        task.wait(SG.config.antiAfk.interval)
        if SG.enabled.antiAfk then
            pcall(function() VirtualUser:Button2Down(Vector2.new(0, 0), Workspace.CurrentCamera.CFrame) end)
            task.wait(0.1)
            pcall(function() VirtualUser:Button2Up(Vector2.new(0, 0), Workspace.CurrentCamera.CFrame) end)
        end
    end
end)

-- =====================================================================
-- MODULE 8: MEM BOOST (offset-direct writes to Humanoid stats)
-- =====================================================================
task.spawn(function()
    while running do
        task.wait(0.1)
        if not SG.enabled.memBoost then continue end
        local h = hum(); if not h then continue end
        if hasMem then
            local a = instAddr(h)
            if a then
                mem.wf(a, OFF.Humanoid.Walkspeed, SG.config.memBoost.walkSpeed)
                mem.wf(a, OFF.Humanoid.JumpPower, SG.config.memBoost.jumpPower)
                mem.wb(a, OFF.Humanoid.UseJumpPower, true)
            end
        else
            pcall(function() h.WalkSpeed = SG.config.memBoost.walkSpeed; h.JumpPower = SG.config.memBoost.jumpPower; h.UseJumpPower = true end)
        end
    end
end)

-- =====================================================================
-- CUSTOM UI
-- =====================================================================
local function buildUI()
    local THEME = {
        bg=Color3.fromRGB(10,12,18), panel=Color3.fromRGB(18,20,28),
        panel2=Color3.fromRGB(26,28,40), border=Color3.fromRGB(50,56,80),
        text=Color3.fromRGB(235,237,245), subtext=Color3.fromRGB(130,138,160),
        accent=Color3.fromRGB(0,230,160), accent2=Color3.fromRGB(0,180,120),
        off=Color3.fromRGB(55,60,80),
    }
    local function corner(p,r) local c=Instance.new("UICorner"); c.CornerRadius=UDim.new(0,r or 6); c.Parent=p; return c end
    local function stroke(p,col,t) local s=Instance.new("UIStroke"); s.Color=col or THEME.border; s.Thickness=t or 1; s.ApplyStrokeMode=Enum.ApplyStrokeMode.Border; s.Parent=p; return s end
    local function pad(p,x,y) local u=Instance.new("UIPadding"); u.PaddingTop=UDim.new(0,y or x); u.PaddingBottom=UDim.new(0,y or x); u.PaddingLeft=UDim.new(0,x); u.PaddingRight=UDim.new(0,x); u.Parent=p end
    local function vlist(p,g) local l=Instance.new("UIListLayout"); l.FillDirection=Enum.FillDirection.Vertical; l.Padding=UDim.new(0,g or 8); l.SortOrder=Enum.SortOrder.LayoutOrder; l.Parent=p end

    local gui = Instance.new("ScreenGui")
    gui.Name = "SafeGuard_"..math.random(10000,99999); gui.IgnoreGuiInset=true; gui.ResetOnSpawn=false; gui.ZIndexBehavior=Enum.ZIndexBehavior.Sibling
    local function tryParent(c) if not c then return false end; local ok=pcall(function() gui.Parent=c end); return ok and gui.Parent==c end
    local parented=false
    if type(gethui)=="function" then local ok,h=pcall(gethui); if ok and h then parented=tryParent(h) end end
    if not parented and type(get_hidden_gui)=="function" then local ok,h=pcall(get_hidden_gui); if ok and h then parented=tryParent(h) end end
    if not parented then parented=tryParent(CoreGui) end
    if not parented then parented=tryParent(lp:FindFirstChildOfClass("PlayerGui") or lp:WaitForChild("PlayerGui")) end

    local main = Instance.new("Frame")
    main.Size=UDim2.new(0,560,0,460); main.Position=UDim2.new(0.5,-280,0.5,-230)
    main.BackgroundColor3=THEME.bg; main.BorderSizePixel=0; main.Parent=gui
    corner(main,10); stroke(main,THEME.border,1)

    local tb = Instance.new("Frame"); tb.Size=UDim2.new(1,0,0,40); tb.BackgroundColor3=THEME.panel; tb.BorderSizePixel=0; tb.Parent=main; corner(tb,10)
    local tbMask=Instance.new("Frame"); tbMask.Size=UDim2.new(1,0,0.6,0); tbMask.Position=UDim2.new(0,0,0.5,0); tbMask.BackgroundColor3=THEME.panel; tbMask.BorderSizePixel=0; tbMask.Parent=tb
    local tbLine=Instance.new("Frame"); tbLine.Size=UDim2.new(1,0,0,1); tbLine.Position=UDim2.new(0,0,1,-1); tbLine.BackgroundColor3=THEME.border; tbLine.BorderSizePixel=0; tbLine.Parent=tb
    local shield=Instance.new("Frame"); shield.Size=UDim2.new(0,10,0,10); shield.Position=UDim2.new(0,14,0.5,-5); shield.BackgroundColor3=THEME.accent; shield.BorderSizePixel=0; shield.Parent=tb; corner(shield,2)
    local brand=Instance.new("TextLabel"); brand.Size=UDim2.new(1,-200,1,0); brand.Position=UDim2.new(0,32,0,0); brand.BackgroundTransparency=1; brand.Text="SafeGuard"; brand.Font=Enum.Font.GothamBlack; brand.TextSize=14; brand.TextColor3=THEME.text; brand.TextXAlignment=Enum.TextXAlignment.Left; brand.Parent=tb
    local status=Instance.new("TextLabel"); status.Size=UDim2.new(0,180,1,0); status.Position=UDim2.new(1,-190,0,0); status.BackgroundTransparency=1; status.Text=(hasMem and "MEM+" or "API") .. " · [9] minimize"; status.Font=Enum.Font.Gotham; status.TextSize=11; status.TextColor3=THEME.subtext; status.TextXAlignment=Enum.TextXAlignment.Right; status.Parent=tb

    local dragging,dragStart,startPos=false,nil,nil
    tb.InputBegan:Connect(function(i) if i.UserInputType==Enum.UserInputType.MouseButton1 or i.UserInputType==Enum.UserInputType.Touch then dragging=true; dragStart=i.Position; startPos=main.Position end end)
    tb.InputEnded:Connect(function(i) if i.UserInputType==Enum.UserInputType.MouseButton1 or i.UserInputType==Enum.UserInputType.Touch then dragging=false end end)
    UIS.InputChanged:Connect(function(i)
        if dragging and (i.UserInputType==Enum.UserInputType.MouseMovement or i.UserInputType==Enum.UserInputType.Touch) then
            local d=i.Position-dragStart; main.Position=UDim2.new(startPos.X.Scale, startPos.X.Offset+d.X, startPos.Y.Scale, startPos.Y.Offset+d.Y)
        end
    end)

    local sidebar=Instance.new("Frame"); sidebar.Size=UDim2.new(0,130,1,-40); sidebar.Position=UDim2.new(0,0,0,40); sidebar.BackgroundColor3=THEME.panel; sidebar.BorderSizePixel=0; sidebar.Parent=main
    local sbL=Instance.new("Frame"); sbL.Size=UDim2.new(0,1,1,0); sbL.Position=UDim2.new(1,-1,0,0); sbL.BackgroundColor3=THEME.border; sbL.BorderSizePixel=0; sbL.Parent=sidebar
    pad(sidebar,10,12); vlist(sidebar,4)

    local content=Instance.new("Frame"); content.Size=UDim2.new(1,-130,1,-40); content.Position=UDim2.new(0,130,0,40); content.BackgroundTransparency=1; content.Parent=main

    local minimized, savedSize = false, main.Size
    track(UIS.InputBegan:Connect(function(i,proc)
        if proc then return end
        if i.KeyCode == Enum.KeyCode.Nine then
            minimized = not minimized
            if minimized then savedSize=main.Size; sidebar.Visible=false; content.Visible=false; main.Size=UDim2.new(0,560,0,40)
            else sidebar.Visible=true; content.Visible=true; main.Size=savedSize end
        end
    end))

    local tabs={}
    local function addTab(label)
        local b=Instance.new("TextButton"); b.Size=UDim2.new(1,0,0,30); b.BackgroundColor3=THEME.panel2; b.BackgroundTransparency=1; b.AutoButtonColor=false; b.Text=""; b.Parent=sidebar; corner(b,6)
        local ind=Instance.new("Frame"); ind.Size=UDim2.new(0,3,0.55,0); ind.Position=UDim2.new(0,0,0.225,0); ind.BackgroundColor3=THEME.accent; ind.BorderSizePixel=0; ind.Visible=false; ind.Parent=b; corner(ind,2)
        local l=Instance.new("TextLabel"); l.Size=UDim2.new(1,-14,1,0); l.Position=UDim2.new(0,14,0,0); l.BackgroundTransparency=1; l.Text=label; l.Font=Enum.Font.GothamMedium; l.TextSize=12; l.TextColor3=THEME.subtext; l.TextXAlignment=Enum.TextXAlignment.Left; l.Parent=b
        local p=Instance.new("ScrollingFrame"); p.Size=UDim2.new(1,0,1,0); p.BackgroundTransparency=1; p.BorderSizePixel=0; p.ScrollBarThickness=3; p.ScrollBarImageColor3=THEME.accent; p.CanvasSize=UDim2.new(0,0,0,0); p.AutomaticCanvasSize=Enum.AutomaticSize.Y; p.Visible=false; p.Parent=content; pad(p,14,14); vlist(p,10)
        local function act() for _, t in ipairs(tabs) do t.p.Visible=false; t.ind.Visible=false; t.b.BackgroundTransparency=1; t.l.TextColor3=THEME.subtext end; p.Visible=true; ind.Visible=true; b.BackgroundTransparency=0.7; l.TextColor3=THEME.text end
        b.MouseButton1Click:Connect(act)
        table.insert(tabs, {p=p, ind=ind, b=b, l=l})
        if #tabs==1 then act() end
        return p
    end

    local function section(page, title)
        local sf=Instance.new("Frame"); sf.Size=UDim2.new(1,0,0,0); sf.AutomaticSize=Enum.AutomaticSize.Y; sf.BackgroundColor3=THEME.panel; sf.BorderSizePixel=0; sf.Parent=page
        corner(sf,8); stroke(sf,THEME.border,1); pad(sf,12,12)
        local sl=Instance.new("Frame"); sl.Size=UDim2.new(1,0,0,0); sl.AutomaticSize=Enum.AutomaticSize.Y; sl.BackgroundTransparency=1; sl.Parent=sf; vlist(sl,8)
        local hd=Instance.new("TextLabel"); hd.Size=UDim2.new(1,0,0,16); hd.BackgroundTransparency=1; hd.Text=string.upper(title); hd.Font=Enum.Font.GothamBold; hd.TextSize=10; hd.TextColor3=THEME.accent; hd.TextXAlignment=Enum.TextXAlignment.Left; hd.LayoutOrder=0; hd.Parent=sl
        return sl
    end

    local function row(parent, h) local r=Instance.new("Frame"); r.Size=UDim2.new(1,0,0,h or 28); r.BackgroundTransparency=1; r.LayoutOrder=#parent:GetChildren(); r.Parent=parent; return r end

    local function addToggle(parent, label, key)
        local r=row(parent, 26)
        local l=Instance.new("TextLabel"); l.Size=UDim2.new(1,-50,1,0); l.BackgroundTransparency=1; l.Font=Enum.Font.Gotham; l.TextSize=12; l.TextColor3=THEME.text; l.TextXAlignment=Enum.TextXAlignment.Left; l.Text=label; l.Parent=r
        local state = SG.enabled[key] and true or false
        local t=Instance.new("TextButton"); t.Size=UDim2.new(0,34,0,18); t.Position=UDim2.new(1,-34,0.5,-9); t.BackgroundColor3=state and THEME.accent or THEME.off; t.BorderSizePixel=0; t.AutoButtonColor=false; t.Text=""; t.Parent=r; corner(t,9)
        local k=Instance.new("Frame"); k.Size=UDim2.new(0,14,0,14); k.Position=state and UDim2.new(1,-16,0.5,-7) or UDim2.new(0,2,0.5,-7); k.BackgroundColor3=Color3.fromRGB(240,240,240); k.BorderSizePixel=0; k.Parent=t; corner(k,7)
        local function apply() TweenService:Create(t, TweenInfo.new(0.12), {BackgroundColor3=state and THEME.accent or THEME.off}):Play(); TweenService:Create(k, TweenInfo.new(0.12), {Position=state and UDim2.new(1,-16,0.5,-7) or UDim2.new(0,2,0.5,-7)}):Play() end
        t.MouseButton1Click:Connect(function() state=not state; SG.enabled[key]=state; apply() end)
    end

    local function addSlider(parent, label, path, min, max, round)
        local keys={}
        for k in string.gmatch(path, "[^%.]+") do keys[#keys+1]=k end
        local function get() local v=SG.config; for _,k in ipairs(keys) do v=v[k] end; return v end
        local function set(v) local t=SG.config; for i=1,#keys-1 do t=t[keys[i]] end; t[keys[#keys]]=v end
        local r=row(parent, 44)
        local l=Instance.new("TextLabel"); l.Size=UDim2.new(1,-70,0,16); l.BackgroundTransparency=1; l.Font=Enum.Font.Gotham; l.TextSize=12; l.TextColor3=THEME.text; l.TextXAlignment=Enum.TextXAlignment.Left; l.Text=label; l.Parent=r
        local vl=Instance.new("TextLabel"); vl.Size=UDim2.new(0,70,0,16); vl.Position=UDim2.new(1,-70,0,0); vl.BackgroundTransparency=1; vl.Font=Enum.Font.GothamBold; vl.TextSize=12; vl.TextColor3=THEME.accent; vl.TextXAlignment=Enum.TextXAlignment.Right; vl.Parent=r
        local trk=Instance.new("Frame"); trk.Size=UDim2.new(1,0,0,4); trk.Position=UDim2.new(0,0,0,28); trk.BackgroundColor3=THEME.off; trk.BorderSizePixel=0; trk.Parent=r; corner(trk,2)
        local fil=Instance.new("Frame"); fil.Size=UDim2.new(0,0,1,0); fil.BackgroundColor3=THEME.accent; fil.BorderSizePixel=0; fil.Parent=trk; corner(fil,2)
        local kn=Instance.new("Frame"); kn.Size=UDim2.new(0,12,0,12); kn.Position=UDim2.new(0,-6,0.5,-6); kn.BackgroundColor3=Color3.fromRGB(245,245,245); kn.BorderSizePixel=0; kn.ZIndex=2; kn.Parent=trk; corner(kn,6)
        local function fmt(v) if round==0 then return tostring(math.floor(v+0.5)) end; return string.format("%."..round.."f",v) end
        local function apply(v) v=math.clamp(v,min,max); if round==0 then v=math.floor(v+0.5) else local m=10^round; v=math.floor(v*m+0.5)/m end; set(v); local t=(max>min) and (v-min)/(max-min) or 0; fil.Size=UDim2.new(t,0,1,0); kn.Position=UDim2.new(t,-6,0.5,-6); vl.Text=fmt(v) end
        apply(get())
        local sd=false
        local function fx(x) local ax=trk.AbsolutePosition.X; local aw=math.max(1,trk.AbsoluteSize.X); apply(min+(max-min)*math.clamp((x-ax)/aw,0,1)) end
        trk.InputBegan:Connect(function(i) if i.UserInputType==Enum.UserInputType.MouseButton1 or i.UserInputType==Enum.UserInputType.Touch then sd=true; fx(i.Position.X) end end)
        kn.InputBegan:Connect(function(i) if i.UserInputType==Enum.UserInputType.MouseButton1 or i.UserInputType==Enum.UserInputType.Touch then sd=true end end)
        UIS.InputChanged:Connect(function(i) if sd and (i.UserInputType==Enum.UserInputType.MouseMovement or i.UserInputType==Enum.UserInputType.Touch) then fx(i.Position.X) end end)
        UIS.InputEnded:Connect(function(i) if i.UserInputType==Enum.UserInputType.MouseButton1 or i.UserInputType==Enum.UserInputType.Touch then sd=false end end)
    end

    local function addDropdown(parent, label, values, path)
        local keys={}
        for k in string.gmatch(path, "[^%.]+") do keys[#keys+1]=k end
        local function get() local v=SG.config; for _,k in ipairs(keys) do v=v[k] end; return v end
        local function set(v) local t=SG.config; for i=1,#keys-1 do t=t[keys[i]] end; t[keys[#keys]]=v end
        local r=row(parent, 26)
        local l=Instance.new("TextLabel"); l.Size=UDim2.new(1,-120,1,0); l.BackgroundTransparency=1; l.Font=Enum.Font.Gotham; l.TextSize=12; l.TextColor3=THEME.text; l.TextXAlignment=Enum.TextXAlignment.Left; l.Text=label; l.Parent=r
        local b=Instance.new("TextButton"); b.Size=UDim2.new(0,110,0,22); b.Position=UDim2.new(1,-110,0.5,-11); b.BackgroundColor3=THEME.panel2; b.BorderSizePixel=0; b.AutoButtonColor=false; b.Font=Enum.Font.Gotham; b.TextSize=11; b.TextColor3=THEME.text; b.Text=tostring(get()).."  ▾"; b.Parent=r; corner(b,5); stroke(b,THEME.border,1)
        local open,menu=false,nil
        local function close() if menu then menu:Destroy(); menu=nil end; open=false end
        b.MouseButton1Click:Connect(function()
            if open then close(); return end
            open=true; menu=Instance.new("Frame"); local ap=b.AbsolutePosition
            menu.Size=UDim2.new(0,110,0,math.min(#values,6)*22+6); menu.Position=UDim2.new(0,ap.X,0,ap.Y+24); menu.BackgroundColor3=THEME.panel2; menu.BorderSizePixel=0; menu.ZIndex=50; menu.Parent=gui
            corner(menu,5); stroke(menu,THEME.border,1); pad(menu,3,3); vlist(menu,0)
            for _, v in ipairs(values) do
                local it=Instance.new("TextButton"); it.Size=UDim2.new(1,0,0,20); it.BackgroundColor3=THEME.accent; it.BackgroundTransparency=1; it.AutoButtonColor=false; it.Font=Enum.Font.Gotham; it.TextSize=11; it.TextColor3=v==get() and THEME.accent or THEME.text; it.Text=tostring(v); it.ZIndex=51; it.Parent=menu; corner(it,4)
                it.MouseEnter:Connect(function() it.BackgroundTransparency=0.8 end)
                it.MouseLeave:Connect(function() it.BackgroundTransparency=1 end)
                it.MouseButton1Click:Connect(function() set(v); b.Text=tostring(v).."  ▾"; close() end)
            end
        end)
    end

    local function addButton(parent, label, cb)
        local r=row(parent, 28)
        local b=Instance.new("TextButton"); b.Size=UDim2.new(1,0,1,0); b.BackgroundColor3=THEME.panel2; b.BorderSizePixel=0; b.AutoButtonColor=false; b.Font=Enum.Font.GothamMedium; b.TextSize=12; b.TextColor3=THEME.text; b.Text=label; b.Parent=r; corner(b,6); stroke(b,THEME.border,1)
        b.MouseEnter:Connect(function() TweenService:Create(b, TweenInfo.new(0.12), {BackgroundColor3=THEME.accent}):Play(); b.TextColor3=Color3.new(1,1,1) end)
        b.MouseLeave:Connect(function() TweenService:Create(b, TweenInfo.new(0.12), {BackgroundColor3=THEME.panel2}):Play(); b.TextColor3=THEME.text end)
        b.MouseButton1Click:Connect(function() pcall(cb) end)
    end

    -- Combat
    local combat = addTab("Combat")
    local sa = section(combat, "Silent Aim")
    addToggle(sa, "Silent aim", "silentAim")
    addDropdown(sa, "Target part", {"HumanoidRootPart","Head","UpperTorso"}, "silentAim.targetPart")
    addSlider(sa, "Hit chance (%)", "silentAim.hitChance", 1, 100, 0)
    local al = section(combat, "Aimlock")
    addToggle(al, "Aimlock", "aimlock")
    addDropdown(al, "Target part", {"Head","HumanoidRootPart","UpperTorso"}, "aimlock.targetPart")
    addSlider(al, "Smooth X", "aimlock.smoothX", 1, 20, 1)
    addSlider(al, "Smooth Y", "aimlock.smoothY", 1, 20, 1)
    addSlider(al, "Sensitivity", "aimlock.sensitivity", 0.1, 5, 2)
    addSlider(al, "FOV (px)", "aimlock.fov", 20, 500, 0)
    local mb = section(combat, "Magic Bullet")
    addToggle(mb, "Magic bullet", "magicBullet")
    addSlider(mb, "FOV (px)", "magicBullet.fovPx", 1, 50, 0)

    -- Visuals
    local vis = addTab("Visuals")
    local esp = section(vis, "ESP")
    addToggle(esp, "Box", "espBox")
    addToggle(esp, "Health bar", "espHealth")
    addToggle(esp, "Name", "espName")
    addToggle(esp, "Distance", "espDistance")
    addToggle(esp, "Skeleton", "espSkeleton")
    addSlider(esp, "Max distance (studs)", "esp.maxDistance", 50, 10000, 0)

    -- Movement
    local mv = addTab("Movement")
    local ms = section(mv, "Memory Boost")
    addToggle(ms, "Enable (writes Humanoid via offsets)", "memBoost")
    addSlider(ms, "WalkSpeed", "memBoost.walkSpeed", 16, 300, 0)
    addSlider(ms, "JumpPower", "memBoost.jumpPower", 50, 500, 0)

    -- Building
    local build = addTab("Building")
    local bs = section(build, "Building Tools")
    addToggle(bs, "Auto place", "buildAutoPlace")
    addToggle(bs, "Turbo build", "buildTurbo")
    addToggle(bs, "Instant edit", "buildInstantEdit")

    -- Desync
    local ds = addTab("Desync")
    local dss = section(ds, "Desync")
    addToggle(dss, "Enabled", "desync")
    addDropdown(dss, "Mode", {"jitter","lag","fake-lag"}, "desync.mode")
    addSlider(dss, "Amount", "desync.amount", 1, 100, 0)

    -- Misc
    local misc = addTab("Misc")
    local mcs = section(misc, "Anti-AFK")
    addToggle(mcs, "Enabled", "antiAfk")
    addSlider(mcs, "Interval (sec)", "antiAfk.interval", 10, 300, 0)
    local sc = section(misc, "Script")
    addButton(sc, "Unload SafeGuard", function() SG.unload() end)

    return gui
end

SG._ui = buildUI()
pcall(function() game:GetService("StarterGui"):SetCore("SendNotification", { Title="SafeGuard", Text=(hasMem and "MEM+ loaded" or "API loaded") .. " — [9] menu", Duration=4 }) end)
print("[SG] SafeGuard v2 loaded — offsets " .. (hasMem and "LIVE" or "FALLBACK to Instance API"))

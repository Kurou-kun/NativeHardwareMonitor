function Update()
	-- PATH 1 (since v1): one plugin measure per data point. Every metric a
	-- script wants needs its own measure declared in the skin.
	local usageNum = SKIN:GetMeasure('Measure_Usage'):GetValue()          -- number path
	local nameStr  = SKIN:GetMeasure('Measure_Name'):GetStringValue()     -- string path
	local voltNum  = SKIN:GetMeasure('Measure_Voltage'):GetValue()        -- sentinel path (-1 unsupported)

	-- PATH 2 (since v2.1): section variables on the single [NHM] hub measure.
	-- Any metric, any device, no measure per data point. SKIN:ReplaceVariables
	-- resolves plugin section variables exactly as a meter option would, so the
	-- script decides what to read at runtime instead of the skin declaring it.
	-- The third argument is the decimal count: 15 keeps every digit, so the two
	-- paths below can be compared exactly. Without it they print to 3 decimals,
	-- which is the right default for a meter but would look like a mismatch here.
	local usageSv = tonumber(SKIN:ReplaceVariables('[NHM:GPU(usage,,15)]'))
	local nameSv  = SKIN:ReplaceVariables('[NHM:GPU(name)]')
	local voltSv  = tonumber(SKIN:ReplaceVariables('[NHM:GPU(voltage,,15)]'))

	-- A metric the skin never declared a measure for, read on the spot:
	local vramSv  = tonumber(SKIN:ReplaceVariables('[NHM:GPU(vramused)]'))
	local cpuSv   = tonumber(SKIN:ReplaceVariables('[NHM:CPU(usage)]'))

	-- print() writes to the Rainmeter log (About window) -> lets us confirm the
	-- actual values crossed the plugin->Lua boundary, which the log otherwise never shows.
	-- The two paths read the same snapshot, so the pairs below must match.
	print('[LuaTest] measures: usage=' .. usageNum .. ' voltage=' .. voltNum .. ' name=' .. nameStr)
	print('[LuaTest] sections: usage=' .. (usageSv or -2) .. ' voltage=' .. (voltSv or -2) .. ' name=' .. nameSv)
	print('[LuaTest] no measure declared for these: vramused=' .. (vramSv or -2) .. ' cpu usage=' .. (cpuSv or -2))

	-- Return both a number and a string value for this Script measure.
	return usageSv or -2, nameSv
end

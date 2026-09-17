# SPDX-License-Identifier: GPL-3.0-or-later
# Bounded external read only: no debugger, thread suspension, or process writes.
[CmdletBinding()]
param([Parameter(Mandatory)][int]$ProcessId,
      [Parameter(Mandatory)][string]$ExpectedExecutable)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class VR2ReadOnlyProcess {
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] buffer, UIntPtr size, out UIntPtr read);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool CloseHandle(IntPtr handle);
}
'@
$target = Get-Process -Id $ProcessId
$module = $target.MainModule
if ([IO.Path]::GetFullPath($module.FileName) -ne [IO.Path]::GetFullPath($ExpectedExecutable) -or
    $module.ModuleName -ne 'SkyrimVR.exe') { throw 'Process executable identity mismatch.' }
$baseAddress = $module.BaseAddress.ToInt64()
$started = $target.StartTime.ToUniversalTime().ToString('o')
$handle = [VR2ReadOnlyProcess]::OpenProcess(0x1010, $false, $ProcessId) # QUERY_LIMITED_INFORMATION | VM_READ only
if ($handle -eq [IntPtr]::Zero) { throw "Read-only process open failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())" }
function Read-Rva([long]$Rva,[int]$Count) {
    if ($Count -lt 1 -or $Count -gt 128 -or $Rva -lt 0 -or $Rva + $Count -gt $module.ModuleMemorySize) { throw 'Read exceeds bounded image region.' }
    return ,(Read-Address ($baseAddress+$Rva) $Count)
}
function Read-Address([long]$Address,[int]$Count) {
    if ($Count -lt 1 -or $Count -gt 128 -or $Address -lt 0x10000 -or $Address -gt 0x00007FFFFFFFFFFF) { throw 'Read exceeds bounded address range.' }
    $buffer = [byte[]]::new($Count)
    $received = [UIntPtr]::Zero
    if (-not [VR2ReadOnlyProcess]::ReadProcessMemory($handle,[IntPtr]$Address,$buffer,[UIntPtr]$Count,[ref]$received) -or $received.ToUInt64() -ne $Count) { throw 'Exact read-only image read failed.' }
    return ,$buffer
}
try {
    $tableBefore = Read-Rva 0x1866248 32
    $open = Read-Rva 0xF20B40 12
    $ctor = Read-Rva 0xF217F0 10
    # Exact VR manager singleton from SKSEVR ScaleformLoader.cpp; bounded
    # loader/implementation pointer chain from the installed CommonLib headers.
    $manager = [BitConverter]::ToInt64((Read-Rva 0x2FEA518 8),0)
    $managerBytes = Read-Address $manager 64
    $loader = [BitConverter]::ToInt64($managerBytes,8)
    $loaderBytes = Read-Address $loader 32
    $loaderTable = [BitConverter]::ToInt64($loaderBytes,0)
    $loaderSlots = Read-Address $loaderTable 48
    $impl = [BitConverter]::ToInt64($loaderBytes,8)
    $implBytes = Read-Address $impl 64
    $implStateSlots = Read-Address ([BitConverter]::ToInt64($implBytes,0x10)) 40
    $bag = [BitConverter]::ToInt64($implBytes,0x28)
    $bagBytes = Read-Address $bag 96
    $bagStateSlots = Read-Address ([BitConverter]::ToInt64($bagBytes,0x10)) 40
    # Statically decoded F7C8B0: state bag hash storage at +30, header
    # mask at +8, then 24-byte entries. Observe ONLY the type-10 bucket,
    # with a maximum of eight links; never enumerate process memory.
    $hashStorage = [BitConverter]::ToInt64($bagBytes,0x30)
    $header = Read-Address $hashStorage 16
    $mask = [BitConverter]::ToInt64($header,8)
    if ($mask -lt 0 -or $mask -gt 255) { throw 'Unqualified state table bounds.' }
    $index = 10 -band $mask
    $openerObservation = $null
    for ($link=0; $link -lt 8; $link++) {
        $entry = Read-Address ($hashStorage+16+24*$index) 24
        $next = [BitConverter]::ToInt64($entry,0)
        if ($next -eq -2) { break }
        $state = [BitConverter]::ToInt64($entry,16)
        $stateBytes = Read-Address $state 24
        $stateType = [BitConverter]::ToInt32($stateBytes,16)
        if ($stateType -eq 10) {
            $stateTable = [BitConverter]::ToInt64($stateBytes,0)
            $stateSlots = Read-Address $stateTable 32
            $openerObservation = [ordered]@{ address=('0x{0:X}' -f $state); type=$stateType;
                vtable=('0x{0:X}' -f $stateTable); vtableRva=('0x{0:X}' -f ($stateTable-$baseAddress));
                slots=@(0..3 | ForEach-Object { '0x{0:X}' -f [BitConverter]::ToInt64($stateSlots,$_ * 8) }) }
            $stateOwner = @($target.Modules | Where-Object { $_.BaseAddress.ToInt64() -le $stateTable -and $_.BaseAddress.ToInt64()+$_.ModuleMemorySize -gt $stateTable })
            if ($stateOwner.Count -ne 1) { throw 'Opener vtable owner not uniquely identified.' }
            $openerObservation.owner = $stateOwner[0].FileName
            $openerObservation.ownerSha256 = (Get-FileHash -LiteralPath $stateOwner[0].FileName).Hash
            $openerObservation.ownerRelativeVtable = '0x{0:X}' -f ($stateTable-$stateOwner[0].BaseAddress.ToInt64())
            $codeAddress = [BitConverter]::ToInt64($stateSlots,24)
            if ($codeAddress -lt $stateOwner[0].BaseAddress.ToInt64() -or $codeAddress+192 -gt $stateOwner[0].BaseAddress.ToInt64()+$stateOwner[0].ModuleMemorySize) { throw 'Opener function outside qualified owner image.' }
            $code = [byte[]]((Read-Address $codeAddress 96)+(Read-Address ($codeAddress+96) 96))
            $openerObservation.openFileEx192ByteSha256 = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($code))
            if ([BitConverter]::ToInt64((Read-Address $state 8),0) -ne $stateTable) { throw 'Opener object changed during observation.' }
            break
        }
        if ($next -eq -1) { break }
        if ($next -lt 0 -or $next -gt $mask) { throw 'State table link out of range.' }
        $index = $next
    }
    $tableAfter = Read-Rva 0x1866248 32
    if ([Convert]::ToHexString($tableBefore) -ne [Convert]::ToHexString($tableAfter)) { throw 'Opener table changed during observation.' }
    [ordered]@{ capturedUtc=[DateTimeOffset]::UtcNow.ToString('o'); processId=$ProcessId;
        processStartedUtc=$started; executable=$module.FileName; base=('0x{0:X}' -f $baseAddress);
        access='QUERY_LIMITED_INFORMATION|VM_READ'; debuggerAttached=$false; threadSuspended=$false; writes=$false;
        manager=('0x{0:X}' -f $manager); loader=('0x{0:X}' -f $loader);
        loaderVtableRva=('0x{0:X}' -f ($loaderTable-$baseAddress)); loaderSlots=@(0..5 | ForEach-Object { '0x{0:X}' -f ([BitConverter]::ToInt64($loaderSlots,$_ * 8)-$baseAddress) });
        impl=('0x{0:X}' -f $impl); implWords=@(0..7 | ForEach-Object { '0x{0:X}' -f [BitConverter]::ToInt64($implBytes,$_ * 8) });
        implStateSlots=@(0..4 | ForEach-Object { '0x{0:X}' -f ([BitConverter]::ToInt64($implStateSlots,$_ * 8)-$baseAddress) });
        bag=('0x{0:X}' -f $bag); bagWords=@(0..11 | ForEach-Object { '0x{0:X}' -f [BitConverter]::ToInt64($bagBytes,$_ * 8) });
        bagStateSlots=@(0..4 | ForEach-Object { '0x{0:X}' -f ([BitConverter]::ToInt64($bagStateSlots,$_ * 8)-$baseAddress) });
        fileOpenerState=$openerObservation;
        tableRva='0x1866248'; slots=@(0..3 | ForEach-Object {
            $value=[BitConverter]::ToInt64($tableBefore,$_ * 8);
            [ordered]@{ slot=$_; address=('0x{0:X}' -f $value); relativeToExe=('0x{0:X}' -f ($value-$baseAddress)) }
        }); tableStable=$true; openFileExPrefix=[Convert]::ToHexString($open); memoryFileCtorPrefix=[Convert]::ToHexString($ctor)
    } | ConvertTo-Json -Depth 5
} finally { [void][VR2ReadOnlyProcess]::CloseHandle($handle) }

# TextStream (host side, zero dependencies).
# Queue text into the keyboard RAM buffer, then press TS_PASTE on the keyboard.
#
# Usage:
#   .\textstream-slim.ps1 -Text "hello"
#   .\textstream-slim.ps1 -File .\long.txt            # replace the buffer with the file
#   .\textstream-slim.ps1 -File .\long.txt -Append    # append instead of replacing
#   .\textstream-slim.ps1 -Text "aA1!zZ" -Shift 50    # Shift lead time override (ms)
#
# Long input is streamed with flow control: the script polls STATUS and never
# writes more than the keyboard reports as free, so any length is fine.
# Non-ASCII bytes are skipped (and counted) because the firmware types US layout.
#
# NOTE: -Shift is stored in keyboard RAM and STAYS until power cycle.
# Omitting it does NOT restore the default (0 = keep current value).

param(
  [string]$Text,
  [string]$File,
  [switch]$Append,
  [int]$Chunk = 30,
  [int]$Shift = 0,
  [int]$TimeoutSec = 600
)

if($File -and $PSBoundParameters.ContainsKey('Text')){Write-Host 'give either -Text or -File, not both';exit 2}
if($Chunk -lt 1 -or $Chunk -gt 30){Write-Host 'Chunk must be 1..30';exit 2}
if($Shift -lt 0 -or $Shift -gt 65535){Write-Host 'Shift must be 0..65535';exit 2}
if($TimeoutSec -lt 1){Write-Host 'TimeoutSec must be >= 1';exit 2}

$skipped=0
if($File){
  $full=Resolve-Path -LiteralPath $File -ErrorAction SilentlyContinue
  if(-not $full){Write-Host "file not found: $File";exit 2}
  $raw=[byte[]][IO.File]::ReadAllBytes($full.Path)
  # strip a UTF-8 BOM if the file has one (guard the 3-byte case: PS ranges count downwards)
  if($raw.Length -ge 3 -and $raw[0] -eq 0xEF -and $raw[1] -eq 0xBB -and $raw[2] -eq 0xBF){
    if($raw.Length -eq 3){$raw=[byte[]]@()}else{$raw=[byte[]]$raw[3..($raw.Length-1)]}
  }
  if($raw.Length -eq 0){Write-Host 'file is empty';exit 2}
  # keep TAB / LF / CR / printable ASCII; everything else (UTF-8 CJK, BOM leftovers) is dropped
  $b=New-Object byte[] $raw.Length
  $n=0
  foreach($c in $raw){
    if($c -eq 9 -or $c -eq 10 -or $c -eq 13 -or ($c -ge 32 -and $c -le 126)){$b[$n]=$c;$n++}else{$skipped++}
  }
  if($n -eq 0){Write-Host 'nothing printable in file';exit 2}
  if($n -lt $b.Length){$b=[byte[]]$b[0..($n-1)]}
  if($skipped -gt 0){Write-Host "skipped $skipped non-ASCII byte(s)"}
} else {
  if($null -eq $Text){Write-Host 'give -Text or -File';exit 2}
  if($Text -match '[^\x09\x0A\x0D\x20-\x7E]'){Write-Host 'non-ASCII input';exit 2}
  $b=[Text.Encoding]::ASCII.GetBytes($Text)
  if($b.Length -eq 0){Write-Host 'nothing to send';exit 2}
}

# The class is compiled once per PowerShell session; a fixed name would collide on a
# second run in the same window (and silently keep an OLD definition, lacking Read).
# So: versioned name + only compile when that exact version is absent.
if(-not ('TStreamV25' -as [type])){
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class TStreamV25{
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr CreateFileW(string n,uint a,uint s,IntPtr q,uint d,uint f,IntPtr t);
 [DllImport("kernel32",SetLastError=true)]static extern bool ReadFile(IntPtr h,byte[] b,int n,out int r,IntPtr o);
 [DllImport("kernel32",SetLastError=true)]static extern bool WriteFile(IntPtr h,byte[] b,int n,out int w,IntPtr o);
 [DllImport("kernel32",SetLastError=true)]static extern uint WaitForSingleObject(IntPtr h,uint ms);
 [DllImport("kernel32")]static extern bool CloseHandle(IntPtr h);
 static IntPtr hw=(IntPtr)(-1),hr=(IntPtr)(-1);
 // Read handle MUST be opened before the first command, and both handles are sync (flags=0).
 public static int Open(string p){
  Stop();
  hw=CreateFileW(p,0x40000000,3,IntPtr.Zero,3,0,IntPtr.Zero);
  if(hw==(IntPtr)(-1))return Marshal.GetLastWin32Error();
  hr=CreateFileW(p,0x80000000,3,IntPtr.Zero,3,0,IntPtr.Zero);
  if(hr==(IntPtr)(-1)){int e=Marshal.GetLastWin32Error();Stop();return e;}
  return 0;
 }
 public static int Send(byte[] d){var b=new byte[33];Array.Copy(d,0,b,1,Math.Min(32,d.Length));int w;return WriteFile(hw,b,33,out w,IntPtr.Zero)?0:Marshal.GetLastWin32Error();}
 public static int Read(byte[] buf,int ms){if(WaitForSingleObject(hr,(uint)ms)!=0)return -1;int r;return ReadFile(hr,buf,33,out r,IntPtr.Zero)?r:-2;}
 public static void Stop(){if(hw!=(IntPtr)(-1)){CloseHandle(hw);hw=(IntPtr)(-1);}if(hr!=(IntPtr)(-1)){CloseHandle(hr);hr=(IntPtr)(-1);}}
}
'@
}

# STATUS is only sent in reply to QUERY: [1]=0x80 [2]=state [3..4]=free [5..6]=typed [7]=err
function Query {
  if([TStreamV25]::Send([byte[]]5) -ne 0){return $null}
  $buf=New-Object byte[] 33
  for($t=0;$t -lt 3;$t++){
    if([TStreamV25]::Read($buf,200) -eq 33 -and $buf[1] -eq 0x80){
      return @{state=[int]$buf[2]; free=([int]$buf[3] -bor ([int]$buf[4] -shl 8)); typed=([int]$buf[5] -bor ([int]$buf[6] -shl 8)); err=[int]$buf[7]}
    }
  }
  return $null
}

$G='{4d1e55b2-f16f-11cf-88cb-001111000030}'
$p=$null
foreach($d in Get-PnpDevice -PresentOnly|? InstanceId -like 'HID\VID_3434&PID_01E0*'){
  $q='\\?\'+($d.InstanceId.ToLower()-replace '\\','#')+"#$G"
  if([TStreamV25]::Open($q)-ne 0){continue}
  if($null -ne (Query)){$p=$q;break}   # only a real STATUS reply proves the link
  [TStreamV25]::Stop()
}
if(-not $p){Write-Host 'TextStream interface not found or not responding (RAW_ENABLE? reflashed?)';exit 1}

# Optional: override the Shift lead time on the keyboard.
# cmd 0x06: [1..2] = shift_ms (16-bit LE); 0 = keep the firmware value.
if($Shift -gt 0){
  $d=[byte[]](6,[byte]($Shift -band 0xFF),[byte](($Shift -shr 8) -band 0xFF))
  if([TStreamV25]::Send($d) -ne 0){[TStreamV25]::Stop();Write-Host 'SET_DELAY failed';exit 1}
}

# Replace the buffer unless -Append was given (otherwise old text stays queued).
if(-not $Append){
  if([TStreamV25]::Send([byte[]]4) -ne 0){[TStreamV25]::Stop();Write-Host 'CLEAR failed';exit 1}
}

# Stream with flow control: poll STATUS, never write more than 'free' bytes.
$off=0
$started=$false
$hinted=$false
$deadline=(Get-Date).AddSeconds($TimeoutSec)
$lastTick=Get-Date
while($off -lt $b.Length){
  $st=Query
  if($null -eq $st){[TStreamV25]::Stop();Write-Host "no STATUS reply after $off / $($b.Length) bytes";exit 1}
  if($st.err -ne 0){Write-Host "keyboard reported error $($st.err) at $off / $($b.Length) bytes"}
  if($st.free -gt 0){
    $n=[Math]::Min($Chunk,$st.free)
    if($n -gt ($b.Length-$off)){$n=$b.Length-$off}
    $k=New-Object byte[] 32;$k[0]=1;$k[1]=$n
    [Array]::Copy($b,$off,$k,2,$n)
    if([TStreamV25]::Send($k) -ne 0){[TStreamV25]::Stop();Write-Host "DATA failed at $off";exit 1}
    $off+=$n
    if(-not $started){
      if([TStreamV25]::Send([byte[]](2,1)) -ne 0){[TStreamV25]::Stop();Write-Host 'START failed';exit 1}
      $started=$true
    }
  } else {
    # Buffer full: only typing can free space, and typing starts on a key press.
    if($st.state -eq 1 -and -not $hinted){
      Write-Host "buffer full ($off / $($b.Length) queued) - press TS_PASTE to keep it draining"
      $hinted=$true
    }
    Start-Sleep -Milliseconds 20
  }
  if((Get-Date) -gt $deadline){
    [TStreamV25]::Stop()
    Write-Host "timeout after $TimeoutSec s ($off / $($b.Length) queued, state=$($st.state))"
    exit 1
  }
  if(((Get-Date)-$lastTick).TotalSeconds -ge 2){
    Write-Host "queued $off / $($b.Length) (state=$($st.state) free=$($st.free) typed=$($st.typed))"
    $lastTick=Get-Date
  }
}
$end=Query
[TStreamV25]::Stop()
if($null -eq $end){Write-Host "queued $($b.Length) bytes - press TS_PASTE on the keyboard (no final STATUS)";exit 0}
if($end.state -eq 2){
  Write-Host "queued $($b.Length) bytes - typing already in progress (typed=$($end.typed), free=$($end.free))"
} else {
  Write-Host "queued $($b.Length) bytes - press TS_PASTE on the keyboard (state=$($end.state), err=$($end.err))"
}
exit 0

# anticap-probe.ps1 - 判断 VDI 会话窗口是否被"防截屏"(SetWindowDisplayAffinity)保护
# 用法：连上目标机、会话窗口可见时运行。全程只读，不修改任何东西。
#   powershell -ExecutionPolicy Bypass -File anticap-probe.ps1
# 判读：
#   WDA_EXCLUDEFROMCAPTURE -> 抓到的图里窗口区域 = 桌面背景（窗口"消失"）
#   WDA_MONITOR            -> 抓到的图里窗口区域 = 纯黑
#   PrintWindow 成功且有内容 -> 未被 WDA 限制（Snipaste 的不可见另有原因，如硬件 overlay）

Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;using System.Text;
public class W{
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32")] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32")] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32")] public static extern IntPtr GetWindowDC(IntPtr h);
  [DllImport("user32")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
  [DllImport("user32")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("gdi32")]  public static extern bool BitBlt(IntPtr d, int x, int y, int w, int h, IntPtr s, int sx, int sy, uint rop);
  [StructLayout(LayoutKind.Sequential)] public struct RECT{ public int L,T,R,B; }
}
'@

$sessionProcs = @('SangforCSClient','SRAPSession','SRAPSession2','SfRemoteAppClient','SfRemoteAppSession')

function Get-SessionWindows {
  $found = New-Object System.Collections.ArrayList
  $cb = [W+EnumProc]{
    param($h,$p)
    $pid2 = 0
    [void][W]::GetWindowThreadProcessId($h,[ref]$pid2)
    $pr = Get-Process -Id $pid2 -ErrorAction SilentlyContinue
    if($pr -and ($sessionProcs -contains $pr.ProcessName)){
      $sb = New-Object System.Text.StringBuilder 512
      [void][W]::GetClassNameW($h,$sb,512)
      $tb = New-Object System.Text.StringBuilder 512
      [void][W]::GetWindowTextW($h,$tb,512)
      $r = New-Object W+RECT
      [void][W]::GetWindowRect($h,[ref]$r)
      [void]$found.Add([pscustomobject]@{
        Hwnd=$h; Proc=$pr.ProcessName; Class=$sb.ToString(); Title=$tb.ToString()
        W=($r.R-$r.L); H=($r.B-$r.T); Visible=[W]::IsWindowVisible($h); Rect=$r })
    }
    return $true
  }
  [void][W]::EnumWindows($cb,[IntPtr]::Zero)
  return $found
}

function Probe($win) {
  Write-Host ("--- {0} [{1}] '{2}' {3}x{4} visible={5}" -f $win.Proc,$win.Class,$win.Title,$win.W,$win.H,$win.Visible)
  if($win.W -le 0 -or $win.H -le 0){ Write-Host "   跳过：无客户区"; return }
  foreach($mode in @('PrintWindow','BitBlt')){
    $bmp = New-Object System.Drawing.Bitmap $win.W, $win.H
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    $ok = $false
    if($mode -eq 'PrintWindow'){ $ok = [W]::PrintWindow($win.Hwnd,$hdc,2) }   # 2 = PW_RENDERFULLCONTENT
    else {
      $src = [W]::GetWindowDC($win.Hwnd)
      $ok = [W]::BitBlt($hdc,0,0,$win.W,$win.H,$src,0,0,0x00CC0020)
      [void][W]::ReleaseDC($win.Hwnd,$src)
    }
    $g.ReleaseHdc($hdc); $g.Dispose()
    # 统计非黑像素比例 + 颜色多样性
    $nonBlack = 0; $total = 0; $colors = @{}
    for($y=0; $y -lt $win.H; $y+=8){
      for($x=0; $x -lt $win.W; $x+=8){
        $c = $bmp.GetPixel($x,$y); $total++
        if($c.R -gt 8 -or $c.G -gt 8 -or $c.B -gt 8){ $nonBlack++ }
        $k = "{0:X2}{1:X2}{2:X2}" -f $c.R,$c.G,$c.B
        if(-not $colors.ContainsKey($k)){ $colors[$k] = 1 } else { $colors[$k]++ }
      }
    }
    $pct = if($total){ [math]::Round(100.0*$nonBlack/$total,1) } else { 0 }
    $out = Join-Path $env:TEMP ("anticap-{0}-{1}.png" -f $win.Proc,$mode)
    $bmp.Save($out,[System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host ("   {0,-11} ok={1,-5} 非黑像素={2,5}%  唯一颜色={3,5}  -> {4}" -f $mode,$ok,$pct,$colors.Count,$out)
    $bmp.Dispose()
  }
}

$wins = Get-SessionWindows
if(-not $wins -or $wins.Count -eq 0){
  Write-Host "没找到会话窗口。先连上目标机、让桌面窗口显示出来再跑。"
  Write-Host "当前 Sangfor 进程："
  Get-Process | Where-Object { $_.Company -like '*Sangfor*' } | Format-Table Id,ProcessName,MainWindowTitle -AutoSize
  exit 1
}
foreach($w in $wins){ Probe $w }

Write-Host ""
Write-Host "判读指南："
Write-Host "  非黑像素≈0% 或 唯一颜色≈1  -> 窗口被防截屏保护（WDA_MONITOR / EXCLUDEFROMCAPTURE）"
Write-Host "  非黑像素高且颜色多样        -> 能抓到内容，Snipaste 的问题另有原因（试试前台+CopyFromScreen）"
Write-Host "  两个模式都失败(ok=False)    -> 窗口用 GPU overlay 渲染，只能用 CopyFromScreen 抓屏幕区域"

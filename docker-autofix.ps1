# ╔══════════════════════════════════════════════════════════════╗
# ║  XP7 OS — Docker Auto-fix Script                           ║
# ║  Ejecutar como Administrador si es posible                 ║
# ╚══════════════════════════════════════════════════════════════╝

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  XP7 OS — Docker Diagnostic & Auto-fix                      ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "SilentlyContinue"
$issues = @()
$fixes = @()

# ═══════════════════════════════════════════════════════════════
# Paso 1: Verificar Docker Desktop
# ═══════════════════════════════════════════════════════════════
Write-Host "[1/5] Verificando Docker Desktop..." -ForegroundColor Yellow

$dockerVersion = docker version 2>&1
if ($LASTEXITCODE -ne 0) {
    $issues += "Docker Desktop no está corriendo"
    Write-Host "  ✗ Docker Desktop NO está corriendo" -ForegroundColor Red
    
    # Intentar iniciar Docker Desktop
    Write-Host "  → Intentando iniciar Docker Desktop..." -ForegroundColor Gray
    Start-Process "C:\Program Files\Docker\Docker\Docker Desktop.exe" -ErrorAction SilentlyContinue
    
    if ($?) {
        Write-Host "  → Docker Desktop iniciado. Espera 30 segundos..." -ForegroundColor Gray
        Start-Sleep -Seconds 30
        $fixes += "Docker Desktop iniciado"
    } else {
        Write-Host "  ✗ No se pudo iniciar Docker Desktop automáticamente" -ForegroundColor Red
        Write-Host "    Abre Docker Desktop manualmente e inicia este script de nuevo" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "  ✓ Docker Desktop está corriendo" -ForegroundColor Green
}

# ═══════════════════════════════════════════════════════════════
# Paso 2: Verificar espacio en disco
# ═══════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "[2/5] Verificando espacio en disco..." -ForegroundColor Yellow

$dockerInfo = docker system df 2>&1 | Out-String
if ($dockerInfo -match "TYPE.*TOTAL.*ACTIVE.*SIZE.*RECLAIMABLE") {
    Write-Host "  ✓ Docker responde" -ForegroundColor Green
    
    # Parsear espacio usado
    if ($dockerInfo -match "RECLAIMABLE\s+(\d+\.?\d*)(GB|MB)") {
        $reclaimable = $matches[1]
        $unit = $matches[2]
        
        if ($unit -eq "GB" -and [double]$reclaimable -gt 5) {
            $issues += "Más de 5 GB de espacio reclaimable"
            Write-Host "  ⚠  $reclaimable $unit de espacio reclaimable" -ForegroundColor Yellow
            Write-Host "     Recomendado limpiar Docker" -ForegroundColor Gray
        } else {
            Write-Host "  ✓ Espacio en Docker OK ($reclaimable $unit reclaimable)" -ForegroundColor Green
        }
    }
} else {
    $issues += "No se pudo obtener info de Docker"
    Write-Host "  ✗ No se pudo obtener información de Docker" -ForegroundColor Red
}

# ═══════════════════════════════════════════════════════════════
# Paso 3: Verificar recursos de Docker
# ═══════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "[3/5] Verificando recursos..." -ForegroundColor Yellow

$dockerInfo = docker info 2>&1
if ($dockerInfo -match "Total Memory:\s+(\d+\.?\d*)(GiB|MiB)") {
    $memory = [double]$matches[1]
    $unit = $matches[2]
    
    if ($unit -eq "GiB" -and $memory -lt 4) {
        $issues += "Memoria de Docker < 4 GB ($memory $unit)"
        Write-Host "  ⚠  Memoria asignada: $memory $unit (recomendado: ≥4 GB)" -ForegroundColor Yellow
    } elseif ($unit -eq "MiB" -and $memory -lt 4096) {
        $issues += "Memoria de Docker < 4 GB"
        Write-Host "  ⚠  Memoria asignada: $memory $unit (recomendado: ≥4 GB)" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ Memoria asignada: $memory $unit" -ForegroundColor Green
    }
}

if ($dockerInfo -match "CPUs:\s+(\d+)") {
    $cpus = [int]$matches[1]
    if ($cpus -lt 2) {
        $issues += "CPUs de Docker < 2 ($cpus)"
        Write-Host "  ⚠  CPUs asignadas: $cpus (recomendado: ≥2)" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ CPUs asignadas: $cpus" -ForegroundColor Green
    }
}

# ═══════════════════════════════════════════════════════════════
# Paso 4: Verificar WSL2 (Windows)
# ═══════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "[4/5] Verificando WSL2..." -ForegroundColor Yellow

$wslStatus = wsl --status 2>&1 | Out-String
if ($wslStatus -match "version 2" -or $wslStatus -match "WSL version: 2") {
    Write-Host "  ✓ WSL2 está activo" -ForegroundColor Green
} else {
    $issues += "WSL2 no está configurado correctamente"
    Write-Host "  ⚠  WSL2 puede no estar configurado" -ForegroundColor Yellow
}

# ═══════════════════════════════════════════════════════════════
# Paso 5: Probar build simple
# ═══════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "[5/5] Probando Docker build..." -ForegroundColor Yellow

# Test simple con hello-world
docker pull hello-world 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Docker pull funciona" -ForegroundColor Green
} else {
    $issues += "Docker pull falla (problema de red?)"
    Write-Host "  ✗ Docker pull falla" -ForegroundColor Red
}

docker run --rm hello-world 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✓ Docker run funciona" -ForegroundColor Green
} else {
    $issues += "Docker run falla"
    Write-Host "  ✗ Docker run falla" -ForegroundColor Red
}

# ═══════════════════════════════════════════════════════════════
# Resumen y recomendaciones
# ═══════════════════════════════════════════════════════════════
Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  RESUMEN                                                     ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

if ($issues.Count -eq 0) {
    Write-Host "✅ TODO OK — Docker está listo para compilar XP7 OS" -ForegroundColor Green
    Write-Host ""
    Write-Host "Ejecuta ahora:" -ForegroundColor White
    Write-Host "  docker build -t xp7os-builder ." -ForegroundColor Cyan
    Write-Host "  docker run --rm -v `"`${PWD}/output:/output`" xp7os-builder" -ForegroundColor Cyan
} else {
    Write-Host "⚠️  Se encontraron $($issues.Count) problema(s):" -ForegroundColor Yellow
    Write-Host ""
    foreach ($issue in $issues) {
        Write-Host "  • $issue" -ForegroundColor Yellow
    }
    Write-Host ""
    
    # ───────────────────────────────────────────────────────────
    # Auto-fix interactivo
    # ───────────────────────────────────────────────────────────
    Write-Host "RECOMENDACIONES:" -ForegroundColor White
    Write-Host ""
    
    if ($issues -like "*Docker Desktop no*") {
        Write-Host "1. Docker Desktop:" -ForegroundColor Yellow
        Write-Host "   → Abrir Docker Desktop manualmente" -ForegroundColor Gray
        Write-Host "   → Esperar a que diga 'Running'" -ForegroundColor Gray
    }
    
    if ($issues -like "*reclaimable*") {
        Write-Host "2. Limpiar Docker:" -ForegroundColor Yellow
        Write-Host "   docker system prune -a --volumes" -ForegroundColor Cyan
        Write-Host ""
        $clean = Read-Host "   ¿Limpiar ahora? (S/N)"
        if ($clean -eq "S" -or $clean -eq "s") {
            Write-Host "   Limpiando..." -ForegroundColor Gray
            docker system prune -a --volumes --force 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) {
                Write-Host "   ✓ Docker limpiado" -ForegroundColor Green
                $fixes += "Docker limpiado"
            } else {
                Write-Host "   ✗ Error al limpiar" -ForegroundColor Red
            }
        }
    }
    
    if ($issues -like "*Memoria*" -or $issues -like "*CPUs*") {
        Write-Host "3. Aumentar recursos:" -ForegroundColor Yellow
        Write-Host "   → Docker Desktop → Settings → Resources" -ForegroundColor Gray
        Write-Host "   → Memory: 8 GB" -ForegroundColor Gray
        Write-Host "   → CPUs: 4" -ForegroundColor Gray
        Write-Host "   → Apply & Restart" -ForegroundColor Gray
    }
    
    if ($issues -like "*WSL2*") {
        Write-Host "4. Reiniciar WSL2:" -ForegroundColor Yellow
        Write-Host "   wsl --shutdown" -ForegroundColor Cyan
        Write-Host ""
        $wsl = Read-Host "   ¿Reiniciar WSL2 ahora? (S/N)"
        if ($wsl -eq "S" -or $wsl -eq "s") {
            Write-Host "   Reiniciando WSL2..." -ForegroundColor Gray
            wsl --shutdown
            Start-Sleep -Seconds 3
            Write-Host "   ✓ WSL2 reiniciado" -ForegroundColor Green
            $fixes += "WSL2 reiniciado"
        }
    }
}

# ═══════════════════════════════════════════════════════════════
# Resumen de fixes aplicados
# ═══════════════════════════════════════════════════════════════
if ($fixes.Count -gt 0) {
    Write-Host ""
    Write-Host "Fixes aplicados:" -ForegroundColor Green
    foreach ($fix in $fixes) {
        Write-Host "  ✓ $fix" -ForegroundColor Green
    }
}

# ═══════════════════════════════════════════════════════════════
# Intentar build?
# ═══════════════════════════════════════════════════════════════
Write-Host ""
$build = Read-Host "¿Intentar compilar XP7 OS ahora? (S/N)"
if ($build -eq "S" -or $build -eq "s") {
    Write-Host ""
    Write-Host "Compilando..." -ForegroundColor Cyan
    Write-Host ""
    
    # Usar Dockerfile mínimo si hay problemas de recursos
    if ($issues -like "*Memoria*" -or $issues -like "*CPUs*") {
        Write-Host "Usando Dockerfile.minimal (más ligero)..." -ForegroundColor Yellow
        docker build -f Dockerfile.minimal -t xp7os-builder .
    } else {
        docker build -t xp7os-builder .
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "╔══════════════════════════════════════════════════════════════╗" -ForegroundColor Green
        Write-Host "║  ✅ BUILD EXITOSO                                           ║" -ForegroundColor Green
        Write-Host "╚══════════════════════════════════════════════════════════════╝" -ForegroundColor Green
        Write-Host ""
        Write-Host "Extraer archivos:" -ForegroundColor White
        Write-Host "  docker run --rm -v `"`${PWD}/output:/output`" xp7os-builder" -ForegroundColor Cyan
    } else {
        Write-Host ""
        Write-Host "╔══════════════════════════════════════════════════════════════╗" -ForegroundColor Red
        Write-Host "║  ✗ BUILD FALLÓ                                              ║" -ForegroundColor Red
        Write-Host "╚══════════════════════════════════════════════════════════════╝" -ForegroundColor Red
        Write-Host ""
        Write-Host "Ver DOCKER_TROUBLESHOOTING.md para más ayuda" -ForegroundColor Yellow
    }
}

Write-Host ""

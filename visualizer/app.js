let trace = null;
let programLines = [];
let currentIndex = 0;
let playTimer = null;
let activeMode = "cycle";
let analysisResults = null;
let selectedAnalysisPredictor = null;

// Static predictor descriptions used by both cycle mode and comparison mode.
const PREDICTOR_DETAILS = {
  "always-not-taken": {
    name: "Always Not Taken",
    type: "Static",
    indexing: "none",
    state: "no table"
  },
  "always-taken": {
    name: "Always Taken",
    type: "Static",
    indexing: "none",
    state: "no table"
  },
  "one-bit": {
    name: "1-bit",
    type: "Local Dynamic",
    indexing: "branch PC",
    state: "1-bit outcome, 0 = NT, 1 = T"
  },
  "two-bit": {
    name: "2-bit",
    type: "Local Dynamic",
    indexing: "branch PC",
    state: "2-bit saturating counter, 00/01 = NT, 10/11 = T"
  },
  gshare: {
    name: "GShare",
    type: "Global Dynamic",
    indexing: "PC XOR GHR",
    state: "2-bit saturating counter, 00/01 = NT, 10/11 = T"
  }
};

const ARCHITECTURE_CONFIG_DEFAULTS = {
  robCapacity: 4,
  intRsCapacity: 2,
  mulRsCapacity: 2,
  fpAddRsCapacity: 2,
  fpMulRsCapacity: 4,
  loadBufferCapacity: 2,
  storeBufferCapacity: 2,
  intFuCount: 2,
  mulFuCount: 1,
  memFuCount: 2,
  fpAddPipelineCount: 1,
  fpAddPipelineDepth: 4,
  fpMulPipelineCount: 1,
  fpMulPipelineDepth: 7,
  intLatency: 1,
  mulLatency: 3,
  loadLatency: 2,
  storeLatency: 2,
  fpAddLatency: 4,
  fpMulLatency: 7,
  fpDivLatency: 10,
  l1dEnabled: false,
  l1dNumSets: 8,
  l1dBlockSizeWords: 4,
  l1dHitLatency: 1,
  l1dMissPenalty: 10
};

const ARCHITECTURE_CONFIG_PRESETS = {
  projectDefault: ARCHITECTURE_CONFIG_DEFAULTS,
  exam: {
    ...ARCHITECTURE_CONFIG_DEFAULTS,
    robCapacity: 32,
    intRsCapacity: 8,
    fpAddRsCapacity: 4,
    fpMulRsCapacity: 4,
    loadBufferCapacity: 4,
    storeBufferCapacity: 4,
    intFuCount: 1,
    memFuCount: 1,
    fpAddPipelineCount: 1,
    fpAddPipelineDepth: 4,
    fpMulPipelineCount: 1,
    fpMulPipelineDepth: 7,
    intLatency: 1,
    loadLatency: 2,
    storeLatency: 2,
    fpAddLatency: 4,
    fpMulLatency: 7
  },
  wide: {
    ...ARCHITECTURE_CONFIG_DEFAULTS,
    robCapacity: 64,
    intRsCapacity: 8,
    mulRsCapacity: 6,
    fpAddRsCapacity: 6,
    fpMulRsCapacity: 8,
    loadBufferCapacity: 8,
    storeBufferCapacity: 8,
    intFuCount: 4,
    mulFuCount: 2,
    memFuCount: 4,
    fpAddPipelineCount: 2,
    fpMulPipelineCount: 2
  }
};

let architectureConfig = loadArchitectureConfig();

// DOM references
const programFileInput = document.getElementById("programFile");
const assemblyInput = document.getElementById("assemblyInput");
const predictorSelect = document.getElementById("predictorSelect");
const runSimulationBtn = document.getElementById("runSimulationBtn");
const runAnalysisBtn = document.getElementById("runAnalysisBtn");
const runStatus = document.getElementById("runStatus");
const cycleModeTab = document.getElementById("cycleModeTab");
const analysisModeTab = document.getElementById("analysisModeTab");
const runnerTitle = document.getElementById("runnerTitle");
const runnerDescription = document.getElementById("runnerDescription");
const analysisOverview = document.getElementById("analysisOverview");
const analysisDetailTabs = document.getElementById("analysisDetailTabs");
const analysisDetails = document.getElementById("analysisDetails");
const architecturePresetSelect = document.getElementById("architecturePresetSelect");
const resetArchitectureConfigBtn = document.getElementById("resetArchitectureConfigBtn");
const architectureSummary = document.getElementById("architectureSummary");
const architectureConfigInputs = Array.from(
  document.querySelectorAll("[data-arch-key]")
);
const l1dConfigFieldset = document.querySelector(".l1d-config-fieldset");
const l1dParamInputs = Array.from(document.querySelectorAll("[data-l1d-param]"));

const prevBtn = document.getElementById("prevBtn");
const nextBtn = document.getElementById("nextBtn");
const playBtn = document.getElementById("playBtn");
const pauseBtn = document.getElementById("pauseBtn");
const cycleSlider = document.getElementById("cycleSlider");

const cycleText = document.getElementById("cycleText");
const pcText = document.getElementById("pcText");

const issuedText = document.getElementById("issuedText");
const cdbText = document.getElementById("cdbText");
const commitText = document.getElementById("commitText");

const robHeadText = document.getElementById("robHeadText");
const robTailText = document.getElementById("robTailText");
const robCountText = document.getElementById("robCountText");

const eventsList = document.getElementById("eventsList");

// Supports both new HTML id="robTable" and old HTML id="robEntries"
const robTable =
  document.getElementById("robTable") ||
  document.getElementById("robEntries");

const lsqEntries = document.getElementById("lsqEntries");
const registerProducers = document.getElementById("registerProducers");
const branchPredictorOverview = document.getElementById("branchPredictorOverview");
const predictorStateTable = document.getElementById("predictorStateTable");
const branchPredictorSummary = document.getElementById("branchPredictorSummary");
const branchPredictorTable = document.getElementById("branchPredictorTable");
const registerState = document.getElementById("registerState");
const memoryState = document.getElementById("memoryState");
const instructionStatusTable = document.getElementById("instructionStatusTable");
const programListing = document.getElementById("programListing");
const performanceStatsPanel = document.getElementById("performanceStatsPanel");
const l1dCacheStatePanel = document.getElementById("l1dCacheStatePanel");

const intRSTable = document.getElementById("intRSTable");
const mulRSTable = document.getElementById("mulRSTable");
const fpAddRSTable = document.getElementById("fpAddRSTable");
const fpMulRSTable = document.getElementById("fpMulRSTable");
const loadBufferTable = document.getElementById("loadBufferTable");
const storeBufferTable = document.getElementById("storeBufferTable");
const fuStatePanel = document.getElementById("fuStatePanel");
const fpPipelinePanel = document.getElementById("fpPipelinePanel");

const programBox = document.getElementById("programBox");
const issueBox = document.getElementById("issueBox");
const rsBox = document.getElementById("rsBox");
const fuBox = document.getElementById("fuBox");
const cdbBox = document.getElementById("cdbBox");
const robBox = document.getElementById("robBox");
const lsqBox = document.getElementById("lsqBox");
const memoryBox = document.getElementById("memoryBox");
const commitBox = document.getElementById("commitBox");
const registerFileBox = document.getElementById("registerFileBox");

const intRsBox = document.getElementById("intRsBox");
const mulRsBox = document.getElementById("mulRsBox");
const fpAddRsBox = document.getElementById("fpAddRsBox");
const fpMulRsBox = document.getElementById("fpMulRsBox");
const loadBufferBox = document.getElementById("loadBufferBox");
const storeBufferBox = document.getElementById("storeBufferBox");
const intFuBox = document.getElementById("intFuBox");
const mulFuBox = document.getElementById("mulFuBox");
const fpAddFuBox = document.getElementById("fpAddFuBox");
const fpMulFuBox = document.getElementById("fpMulFuBox");
const memFuBox = document.getElementById("memFuBox");

const programBoxMain = document.getElementById("programBoxMain");
const programBoxSub = document.getElementById("programBoxSub");
const issueBoxMain = document.getElementById("issueBoxMain");
const issueBoxSub = document.getElementById("issueBoxSub");
const rsBoxMain = document.getElementById("rsBoxMain");
const fuBoxMain = document.getElementById("fuBoxMain");
const fuBoxSub = document.getElementById("fuBoxSub");
const cdbBoxMain = document.getElementById("cdbBoxMain");
const robBoxMain = document.getElementById("robBoxMain");
const robBoxSub = document.getElementById("robBoxSub");
const lsqBoxMain = document.getElementById("lsqBoxMain");
const memoryBoxMain = document.getElementById("memoryBoxMain");
const commitBoxMain = document.getElementById("commitBoxMain");
const intRsBoxMain = document.getElementById("intRsBoxMain");
const mulRsBoxMain = document.getElementById("mulRsBoxMain");
const fpAddRsBoxMain = document.getElementById("fpAddRsBoxMain");
const fpMulRsBoxMain = document.getElementById("fpMulRsBoxMain");
const loadBufferBoxMain = document.getElementById("loadBufferBoxMain");
const storeBufferBoxMain = document.getElementById("storeBufferBoxMain");
const intFuBoxMain = document.getElementById("intFuBoxMain");
const mulFuBoxMain = document.getElementById("mulFuBoxMain");
const fpAddFuBoxMain = document.getElementById("fpAddFuBoxMain");
const fpMulFuBoxMain = document.getElementById("fpMulFuBoxMain");
const memFuBoxMain = document.getElementById("memFuBoxMain");

const arrowProgramIssue = document.getElementById("arrowProgramIssue");
const arrowIssueRS = document.getElementById("arrowIssueRS");
const arrowRSFU = document.getElementById("arrowRSFU");
const arrowFUCDB = document.getElementById("arrowFUCDB");
const arrowCDBROB = document.getElementById("arrowCDBROB");
const arrowRSLSQ = document.getElementById("arrowRSLSQ");
const arrowLSQMemory = document.getElementById("arrowLSQMemory");
const arrowROBCommit = document.getElementById("arrowROBCommit");

const componentBoxes = [
  programBox,
  issueBox,
  rsBox,
  fuBox,
  cdbBox,
  robBox,
  lsqBox,
  memoryBox,
  commitBox,
  registerFileBox,
  intRsBox,
  mulRsBox,
  fpAddRsBox,
  fpMulRsBox,
  loadBufferBox,
  storeBufferBox,
  intFuBox,
  mulFuBox,
  fpAddFuBox,
  fpMulFuBox,
  memFuBox
].filter(Boolean);

const arrows = [
  arrowProgramIssue,
  arrowIssueRS,
  arrowRSFU,
  arrowFUCDB,
  arrowCDBROB,
  arrowRSLSQ,
  arrowLSQMemory,
  arrowROBCommit
].filter(Boolean);

if (programFileInput) {
  programFileInput.addEventListener("change", handleProgramFile);
}

if (runSimulationBtn) {
  runSimulationBtn.addEventListener("click", runSimulationFromInput);
}

if (runAnalysisBtn) {
  runAnalysisBtn.addEventListener("click", runPredictionAnalysis);
}

if (cycleModeTab) {
  cycleModeTab.addEventListener("click", () => setMode("cycle"));
}

if (analysisModeTab) {
  analysisModeTab.addEventListener("click", () => setMode("analysis"));
}

if (predictorSelect) {
  predictorSelect.addEventListener("change", () => {
    if (trace) {
      render();
      return;
    }

    renderPredictorOverview(predictorSelect.value);
    renderPredictorState(null, predictorSelect.value);
  });
}

initializeArchitectureControls();
renderPredictorOverview(predictorSelect ? predictorSelect.value : "two-bit");
renderPredictorState(null, predictorSelect ? predictorSelect.value : "two-bit");
setMode("cycle");

if (prevBtn) {
  prevBtn.addEventListener("click", previousCycle);
}

if (nextBtn) {
  nextBtn.addEventListener("click", nextCycle);
}

if (playBtn) {
  playBtn.addEventListener("click", play);
}

if (pauseBtn) {
  pauseBtn.addEventListener("click", pause);
}

if (cycleSlider) {
  cycleSlider.addEventListener("input", () => {
    if (!trace) return;
    currentIndex = Number(cycleSlider.value);
    render();
  });
}

document.addEventListener("keydown", (event) => {
  if (isEditableTarget(event.target)) return;
  if (activeMode !== "cycle") return;
  if (!trace) return;

  if (event.key === "ArrowLeft") {
    previousCycle();
  } else if (event.key === "ArrowRight") {
    nextCycle();
  } else if (event.key === " ") {
    event.preventDefault();

    if (playTimer) {
      pause();
    } else {
      play();
    }
  }
});

// File loading / API helpers
function handleProgramFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    const assemblyCode = e.target.result;
    programLines = parseProgramLines(assemblyCode);

    if (assemblyInput) {
      assemblyInput.value = assemblyCode;
    }

    render();
  };

  reader.readAsText(file);
}

async function runSimulationFromInput() {
  if (!assemblyInput) return;

  const assemblyCode = assemblyInput.value;

  if (!assemblyCode.trim()) {
    setRunStatus("Paste assembly code or load an .asm file first.", "error");
    return;
  }

  setMode("cycle");
  pause();
  setRunStatus("Running simulator...", "");
  setRunButtonDisabled(true);

  try {
    // Cycle simulation and prediction analysis use the same architecture
    // payload shape so results can be compared under the same machine model.
    const response = await fetch("/run", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        code: assemblyCode,
        assembly: assemblyCode,
        predictor: predictorSelect ? predictorSelect.value : undefined,
        architectureConfig: getArchitectureConfigForRequest()
      })
    });

    const payload = await parseJsonResponse(response);

    if (!response.ok) {
      throw new Error(payload.error || "Simulation failed.");
    }

    loadTrace(payload, parseProgramLines(assemblyCode));
    setRunStatus(`Simulation complete. Loaded ${payload.cycles.length} cycles.`, "success");
  } catch (error) {
    console.error(error);
    setRunStatus(error.message, "error");
  } finally {
    setRunButtonDisabled(false);
  }
}

async function runPredictionAnalysis() {
  if (!assemblyInput) return;

  const assemblyCode = assemblyInput.value;

  if (!assemblyCode.trim()) {
    setRunStatus("Paste assembly code or load an .asm file first.", "error");
    return;
  }

  setMode("analysis");
  pause();
  setRunStatus("Running prediction analysis...", "");
  setRunButtonDisabled(true);

  if (runAnalysisBtn) {
    runAnalysisBtn.disabled = true;
  }

  try {
    // Prediction analysis forwards the selected architecture config to every
    // predictor run on the backend.
    const response = await fetch("/compare-predictors", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        code: assemblyCode,
        assembly: assemblyCode,
        architectureConfig: getArchitectureConfigForRequest()
      })
    });

    const payload = await parseJsonResponse(response);

    if (!response.ok) {
      throw new Error(payload.error || "Prediction analysis failed.");
    }

    analysisResults = payload;
    selectedAnalysisPredictor =
      payload.bestPredictor ||
      payload.results?.find((result) => !result.error)?.predictor ||
      payload.results?.[0]?.predictor ||
      null;
    renderAnalysisResults();
    setRunStatus("Prediction analysis complete.", "success");
  } catch (error) {
    console.error(error);
    setRunStatus(error.message, "error");
  } finally {
    setRunButtonDisabled(false);

    if (runAnalysisBtn) {
      runAnalysisBtn.disabled = false;
    }
  }
}

function loadTrace(parsedTrace, nextProgramLines) {
  if (!parsedTrace.cycles || !Array.isArray(parsedTrace.cycles)) {
    throw new Error("Invalid trace file: expected a cycles array.");
  }

  trace = parsedTrace;

  if (Array.isArray(trace.program)) {
    programLines = normalizeProgramLines(trace.program);
  } else if (Array.isArray(nextProgramLines)) {
    programLines = normalizeProgramLines(nextProgramLines);
  }

  currentIndex = 0;

  if (cycleSlider) {
    cycleSlider.min = 0;
    cycleSlider.max = trace.cycles.length - 1;
    cycleSlider.value = 0;
  }

  renderArchitectureSummary(trace.architectureConfig);
  render();
}

async function parseJsonResponse(response) {
  const responseText = await response.text();

  try {
    return JSON.parse(responseText);
  } catch {
    throw new Error(
      `Backend returned non-JSON response (${response.status}). ` +
      responseText.slice(0, 300)
    );
  }
}

function setMode(mode) {
  activeMode = mode === "analysis" ? "analysis" : "cycle";
  document.body.dataset.mode = activeMode;

  if (cycleModeTab) {
    cycleModeTab.classList.toggle("active", activeMode === "cycle");
  }

  if (analysisModeTab) {
    analysisModeTab.classList.toggle("active", activeMode === "analysis");
  }

  if (runnerTitle) {
    runnerTitle.textContent = activeMode === "analysis"
      ? "Run Prediction Analysis"
      : "Run Simulation";
  }

  if (runnerDescription) {
    runnerDescription.textContent = activeMode === "analysis"
      ? "Compare branch predictor accuracy for the same assembly program."
      : "Paste assembly or load an .asm file, then run the local simulator backend.";
  }

  if (runStatus) {
    runStatus.textContent = activeMode === "analysis"
      ? "Backend endpoint: POST /compare-predictors"
      : "Backend endpoint: POST /run";
    runStatus.classList.remove("success", "error");
  }

  if (activeMode === "analysis") {
    renderAnalysisResults();
  }
}

// Architecture Config
// The selected values are sent with both /run and /compare-predictors. After
// simulation, the backend trace is treated as the source of truth because older
// traces may omit architectureConfig and the backend may fill default values.
function initializeArchitectureControls() {
  syncArchitectureInputs();
  renderArchitectureSummary();

  if (architecturePresetSelect) {
    architecturePresetSelect.addEventListener("change", () => {
      const preset = ARCHITECTURE_CONFIG_PRESETS[architecturePresetSelect.value];
      if (!preset) return;
      architectureConfig = { ...preset };
      persistArchitectureConfig();
      syncArchitectureInputs();
      renderArchitectureSummary();
    });
  }

  if (resetArchitectureConfigBtn) {
    resetArchitectureConfigBtn.addEventListener("click", () => {
      architectureConfig = { ...ARCHITECTURE_CONFIG_DEFAULTS };
      if (architecturePresetSelect) {
        architecturePresetSelect.value = "projectDefault";
      }
      persistArchitectureConfig();
      syncArchitectureInputs();
      renderArchitectureSummary();
    });
  }

  for (const input of architectureConfigInputs) {
    input.addEventListener("input", () => {
      const key = input.dataset.archKey;
      if (!key) return;

      if (input.type === "checkbox") {
        architectureConfig[key] = input.checked;
        persistArchitectureConfig();
        updateArchitecturePresetSelection();
        syncL1DConfigState();
        renderArchitectureSummary();
        return;
      }

      const value = Number.parseInt(input.value, 10);
      if (!Number.isFinite(value)) return;

      architectureConfig[key] = value;
      persistArchitectureConfig();
      updateArchitecturePresetSelection();
      renderArchitectureSummary();
    });
  }
}

function loadArchitectureConfig() {
  try {
    const stored = localStorage.getItem("tomasuloArchitectureConfig");
    if (!stored) {
      return { ...ARCHITECTURE_CONFIG_DEFAULTS };
    }

    return sanitizeArchitectureConfig(JSON.parse(stored));
  } catch {
    return { ...ARCHITECTURE_CONFIG_DEFAULTS };
  }
}

function sanitizeArchitectureConfig(config) {
  const sanitized = { ...ARCHITECTURE_CONFIG_DEFAULTS };

  for (const key of Object.keys(ARCHITECTURE_CONFIG_DEFAULTS)) {
    if (typeof ARCHITECTURE_CONFIG_DEFAULTS[key] === "boolean") {
      sanitized[key] = Boolean(config?.[key]);
      continue;
    }

    const value = Number.parseInt(config?.[key], 10);
    if (Number.isFinite(value)) {
      sanitized[key] = value;
    }
  }

  return sanitized;
}

function persistArchitectureConfig() {
  try {
    localStorage.setItem(
      "tomasuloArchitectureConfig",
      JSON.stringify(architectureConfig)
    );
  } catch {
    // Local storage is optional; the in-memory config still applies.
  }
}

function syncArchitectureInputs() {
  for (const input of architectureConfigInputs) {
    const key = input.dataset.archKey;
    if (!key) continue;

    if (input.type === "checkbox") {
      input.checked = Boolean(architectureConfig[key]);
    } else {
      input.value = architectureConfig[key];
    }
  }

  syncL1DConfigState();
  updateArchitecturePresetSelection();
}

function syncL1DConfigState() {
  const enabled = Boolean(architectureConfig.l1dEnabled);

  if (l1dConfigFieldset) {
    l1dConfigFieldset.classList.toggle("cache-disabled", !enabled);
  }

  for (const input of l1dParamInputs) {
    input.disabled = !enabled;
  }
}

function updateArchitecturePresetSelection() {
  if (!architecturePresetSelect) return;

  const matchingPreset = Object.entries(ARCHITECTURE_CONFIG_PRESETS)
    .find(([, preset]) => architectureConfigsEqual(architectureConfig, preset));

  architecturePresetSelect.value = matchingPreset ? matchingPreset[0] : "";
}

function architectureConfigsEqual(left, right) {
  return Object.keys(ARCHITECTURE_CONFIG_DEFAULTS).every((key) => {
    if (typeof ARCHITECTURE_CONFIG_DEFAULTS[key] === "boolean") {
      return Boolean(left[key]) === Boolean(right[key]);
    }

    return Number(left[key]) === Number(right[key]);
  });
}

function getArchitectureConfigForRequest() {
  // Input listeners keep architectureConfig current; this function is the
  // single request payload path for cycle and prediction-analysis runs.
  return { ...architectureConfig };
}

function getTraceArchitectureConfig() {
  return sanitizeArchitectureConfig(
    trace?.architectureConfig || architectureConfig
  );
}

function renderArchitectureSummary(config = null) {
  if (!architectureSummary) return;

  const activeConfig = sanitizeArchitectureConfig(config || trace?.architectureConfig || architectureConfig);
  architectureSummary.textContent =
    `Architecture: ROB ${activeConfig.robCapacity}, ` +
    `INT FU ${activeConfig.intFuCount}, ` +
    `MEM FU ${activeConfig.memFuCount}, ` +
    `FP_ADD ${activeConfig.fpAddPipelineCount}x${activeConfig.fpAddPipelineDepth}, ` +
    `FP_MUL ${activeConfig.fpMulPipelineCount}x${activeConfig.fpMulPipelineDepth}, ` +
    `L1D ${activeConfig.l1dEnabled ? "enabled" : "disabled"}`;
}

function parseProgramLines(text) {
  const program = [];

  for (const originalLine of text.split(/\r?\n/)) {
    let line = originalLine
      .replace(/#.*$/g, "")
      .replace(/\/\/.*$/g, "")
      .trim();

    if (!line) continue;

    while (line.includes(":")) {
      const colonIndex = line.indexOf(":");
      line = line.slice(colonIndex + 1).trim();

      if (!line) break;
    }

    if (!line) continue;
    // .REG/.MEM are setup-only directives. They may appear in source text but
    // are not executable PCs, so the Program/PC view excludes them.
    if (line.startsWith(".")) continue;

    program.push(line);
  }

  return program;
}

function normalizeProgramLines(lines) {
  return lines
    .map((entry) => {
      if (entry && typeof entry === "object") {
        return String(entry.text ?? entry.rawText ?? "");
      }

      return String(entry);
    })
    .map((line) => line.trim())
    .filter(Boolean);
}

// Cycle controls
function previousCycle() {
  if (!trace) return;
  currentIndex = Math.max(0, currentIndex - 1);
  render();
}

function nextCycle() {
  if (!trace) return;
  currentIndex = Math.min(trace.cycles.length - 1, currentIndex + 1);
  render();
}

function play() {
  if (!trace || playTimer) return;

  playTimer = setInterval(() => {
    if (currentIndex >= trace.cycles.length - 1) {
      pause();
      return;
    }

    currentIndex++;
    render();
  }, 600);
}

function pause() {
  if (playTimer) {
    clearInterval(playTimer);
    playTimer = null;
  }
}

// Main render function
function render() {
  if (!trace) return;

  const cycle = trace.cycles[currentIndex];
  const events = cycle.events || [];
  const rob = cycle.rob || { head: "-", tail: "-", count: 0, entries: [] };
  const robEntries = rob.entries || [];
  const displayROBEntries = getDisplayROBEntries(cycle, robEntries);
  const displayRegisterProducers = getDisplayRegisterProducers(
    cycle,
    cycle.registerProducers || [],
    robEntries
  );
  const finalCleanupApplied = displayROBEntries.length !== robEntries.length;

  if (cycleSlider) {
    cycleSlider.value = currentIndex;
  }

  setText(cycleText, `Cycle ${cycle.cycle}`);
  setText(pcText, `PC: ${cycle.pc}`);

  setText(issuedText, cycle.issuedInstruction || "none");
  setText(cdbText, cycle.cdbBroadcast || "none");
  setText(commitText, cycle.commitEvent || "none");

  setText(robHeadText, finalCleanupApplied ? "-" : `ROB${rob.head}`);
  setText(robTailText, finalCleanupApplied ? "-" : `ROB${rob.tail}`);
  const robCapacity = getROBCapacity(cycle);
  setText(robCountText, `${displayROBEntries.length}/${robCapacity}`);

  renderProgram(cycle);
  renderDatapath(cycle, events);
  renderEvents(events);
  renderROB(displayROBEntries, cycle);
  renderRegisterProducers(displayRegisterProducers);
  renderBranchPredictions(cycle);
  renderReservationStations(cycle);
  renderFUState(cycle);
  renderFPPipelines(cycle);
  renderPerformanceStats();
  renderLSQ(cycle.lsq || []);
  renderL1DCacheState(cycle.l1dCache);
  renderRegisterState(cycle.registers);
  renderMemoryState(cycle.memory);
  renderInstructionStatus(cycle);
}

// Program / PC rendering
// PC highlighting uses real instruction indices from trace.program/status, not
// source line numbers. This keeps setup directives and labels from shifting the
// displayed executable PC list.
function renderProgram(cycle) {
  if (!programListing) return;

  programListing.innerHTML = "";

  if (programLines.length === 0) {
    const empty = document.createElement("div");
    empty.className = "program-line";
    empty.innerHTML = `
      <span class="program-line-number">-</span>
      <span>Load the .asm file to show program lines.</span>
    `;
    programListing.appendChild(empty);
    return;
  }

  const issuedIndex = getIssuedProgramIndex(cycle);

  for (let i = 0; i < programLines.length; i++) {
    const line = document.createElement("div");
    line.className = "program-line";

    if (i === cycle.pc) {
      line.classList.add("current-pc");
    }

    if (i === issuedIndex) {
      line.classList.add("issued-line");
    }

    line.innerHTML = `
      <span class="program-line-number">${i}</span>
      <span>${escapeHtml(programLines[i])}</span>
    `;

    programListing.appendChild(line);
  }
}

function findIssuedProgramIndex(issuedInstruction) {
  if (!issuedInstruction) return -1;

  const normalizedIssued = normalizeInstruction(issuedInstruction);

  for (let i = 0; i < programLines.length; i++) {
    if (normalizeInstruction(programLines[i]) === normalizedIssued) {
      return i;
    }
  }

  return -1;
}

function getIssuedProgramIndex(cycle) {
  if (!cycle.issuedInstruction) return -1;

  if (Array.isArray(trace?.instructionStatus)) {
    const issuedStatus = trace.instructionStatus.find((entry) => {
      return entry.issueCycle === cycle.cycle;
    });

    if (issuedStatus && typeof issuedStatus.pc === "number") {
      return issuedStatus.pc;
    }
  }

  if (typeof cycle.pc === "number") {
    return cycle.pc - 1;
  }

  return findIssuedProgramIndex(cycle.issuedInstruction);
}

// Datapath rendering
function renderDatapath(cycle, events) {
  clearHighlights();

  const hasIssued = Boolean(cycle.issuedInstruction);
  const hasCDB = cycle.cdbBroadcast && cycle.cdbBroadcast !== "none";
  const hasCommit = Boolean(cycle.commitEvent);
  const hasStall = hasEvent(events, "Issue stalled");
  const hasExecutionStart = hasEvent(events, "Execution started");
  const hasExecutionComplete = hasEvent(events, "Execution complete");
  const hasMispredict = hasEvent(events, "Branch misprediction");
  const hasFlush = hasEvent(events, "Flushed");
  const hasBranch = hasEvent(events, "Branch resolved") || hasEvent(events, "Branch prediction");
  const hasMemoryInstruction = isMemoryInstruction(cycle.issuedInstruction);
  const hasLSQ = hasMemoryInstruction || hasEvent(events, "LSQ");
  const hasMemory =
    hasEvent(events, "Memory Commit") ||
    hasEvent(events, "Store result ready") ||
    hasEvent(events, "Load result ready");

  const activeInstructions = Array.isArray(cycle.activeInstructions)
    ? cycle.activeInstructions
    : [];
  const grouped = groupInstructionsByResource(activeInstructions, {
    includeExecuting: false
  });
  const rsState = getRSState(cycle, grouped);
  const waitingCount = Object.values(rsState)
    .reduce((sum, usage) => sum + usage.used, 0);
  const rob = cycle.rob || { head: "-", tail: "-", count: 0 };
  const robCount = rob.count;
  const robCapacity = getROBCapacity(cycle);
  const lsqCount = cycle.lsq ? cycle.lsq.length : 0;
  const issuedResource = getResourceType(cycle.issuedInstruction);
  const activeResources = new Set(
    activeInstructions
      .filter(isReservationStationEntry)
      .map((entry) => getResourceType(entry.rawText))
  );
  const executingResources = new Set(
    activeInstructions
      .filter((entry) => entry.executing)
      .map((entry) => getResourceType(entry.rawText))
  );

  setText(programBoxMain, `PC: ${cycle.pc}`);
  setText(programBoxSub, getProgramLine(cycle.pc));

  setText(issueBoxMain, cycle.issuedInstruction || "none");
  setText(issueBoxSub, hasStall
    ? "issue stalled"
    : hasIssued ? formatResourceLabel(issuedResource) : "instruction issue");

  setText(rsBoxMain, `${waitingCount} waiting`);
  setText(intRsBoxMain, `${rsState.INT.used}/${rsState.INT.capacity} waiting`);
  setText(mulRsBoxMain, `${rsState.MUL.used}/${rsState.MUL.capacity} waiting`);
  setText(fpAddRsBoxMain, `${rsState.FP_ADD.used}/${rsState.FP_ADD.capacity} waiting`);
  setText(fpMulRsBoxMain, `${rsState.FP_MUL.used}/${rsState.FP_MUL.capacity} waiting`);
  setText(loadBufferBoxMain, `${rsState.LOAD.used}/${rsState.LOAD.capacity} waiting`);
  setText(storeBufferBoxMain, `${rsState.STORE.used}/${rsState.STORE.capacity} waiting`);

  setText(
    fuBoxMain,
    hasExecutionStart
      ? getFirstEvent(events, "Execution started")
      : "INT / MUL / FP_ADD / FP_MUL / MEM"
  );

  setText(
    fuBoxSub,
    hasExecutionComplete ? "completed this cycle" : "execution"
  );

  updateDatapathFUCounts(cycle, groupInstructionsByResource(activeInstructions));

  setText(cdbBoxMain, hasCDB ? cycle.cdbBroadcast : "none");

  setText(robBoxMain, `${robCount}/${robCapacity} entries`);
  setText(robBoxSub, `head ROB${rob.head}, tail ROB${rob.tail}`);

  setText(lsqBoxMain, `${lsqCount} entries`);
  setText(memoryBoxMain, hasMemory ? "memory event" : "loads/stores");
  setText(commitBoxMain, cycle.commitEvent || "none");

  if (hasIssued) {
    activate(programBox, arrowProgramIssue, issueBox, arrowIssueRS, rsBox);
    activateResourcePath(issuedResource, "rs");
  }

  if (hasStall) {
    markStall(issueBox, arrowProgramIssue);
  }

  for (const resource of activeResources) {
    activateResourcePath(resource, "rs");
  }

  for (const resource of executingResources) {
    activateResourcePath(resource, "fu");
  }

  if (hasExecutionStart || hasExecutionComplete || executingResources.size > 0) {
    activate(rsBox, arrowRSFU, fuBox);
  }

  if (hasCDB) {
    const cdbResource = getResourceType(cycle.cdbBroadcast);
    activateResourcePath(cdbResource, "fu");
    activate(fuBox, arrowFUCDB, cdbBox, arrowCDBROB, robBox, registerFileBox);
  }

  if (hasCommit) {
    markCommit(robBox, arrowROBCommit, commitBox);
  }

  if (hasBranch) {
    activate(programBox, issueBox, intRsBox, intFuBox);
  }

  if (hasMispredict || hasFlush) {
    markFlush(programBox, issueBox, rsBox, robBox);
  }

  if (hasLSQ) {
    activate(rsBox, arrowRSLSQ, lsqBox, loadBufferBox, storeBufferBox);
  }

  if (hasMemory) {
    activate(lsqBox, arrowLSQMemory, memoryBox, memFuBox);
  }
}

function activateResourcePath(resource, stage) {
  const rsBoxForResource = getRSBoxForResource(resource);
  const fuBoxForResource = getFUBoxForResource(resource);

  if (stage === "fu") {
    activate(fuBoxForResource);
    return;
  }

  activate(rsBoxForResource);
}

function updateDatapathFUCounts(cycle, grouped) {
  const fuState = getFUState(cycle, grouped);

  setText(intFuBoxMain, `${fuState.INT.busy}/${fuState.INT.total} busy`);
  setText(mulFuBoxMain, `${fuState.MUL.busy}/${fuState.MUL.total} busy`);
  setText(fpAddFuBoxMain, formatFUUsageCompact(fuState.FP_ADD));
  setText(fpMulFuBoxMain, formatFUUsageCompact(fuState.FP_MUL));
  setText(memFuBoxMain, `${fuState.MEM.busy}/${fuState.MEM.total} busy`);
}

function clearHighlights() {
  for (const box of componentBoxes) {
    box.classList.remove("active", "stall", "flush", "mispredict", "commit-active");
  }

  for (const arrow of arrows) {
    arrow.classList.remove("active", "stall", "flush", "mispredict", "commit-active");
  }
}

function activate(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("active");
    }
  }
}

function markStall(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("stall");
    }
  }
}

function markFlush(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("flush");
    }
  }
}

function markCommit(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("commit-active");
    }
  }
}

// Event rendering
function renderEvents(events) {
  if (!eventsList) return;

  eventsList.innerHTML = "";

  if (events.length === 0) {
    const li = document.createElement("li");
    li.textContent = "No events this cycle.";
    li.className = "empty";
    eventsList.appendChild(li);
    return;
  }

  for (const event of events) {
    const li = document.createElement("li");
    li.textContent = event;
    li.className = getEventClass(event);
    eventsList.appendChild(li);
  }
}

// ROB rendering
function renderROB(entries, cycle) {
  if (!robTable) {
    console.error("Missing ROB container. Expected id='robTable' or id='robEntries'.");
    return;
  }

  const capacity = getROBCapacity(cycle);

  let html = `
    <table class="rob-table">
      <thead>
        <tr>
          <th>Slot</th>
          <th>Markers</th>
          <th>Busy</th>
          <th>Instruction</th>
          <th>Ready</th>
          <th>Destination</th>
          <th>Value</th>
          <th>Memory Write</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (let slot = 0; slot < capacity; slot++) {
    const entry = entries.find((e) => e.robTag === slot);
    const markers = getROBMarkers(slot);
    const markerClass = getROBMarkerClass(slot);

    if (!entry) {
      html += `
        <tr class="empty-row ${markerClass}">
          <td class="rob-slot">ROB${slot}</td>
          <td>${formatROBMarkers(markers)}</td>
          <td>no</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
        </tr>
      `;
      continue;
    }

    const rowClass = entry.ready ? "ready-row" : "not-ready-row";

    const destination =
      entry.writesRegister && entry.destinationRegister >= 0
        ? `R${entry.destinationRegister}`
        : "-";

    const memoryWrite = entry.writesMemory
      ? `Mem[${entry.memoryAddress}] = ${entry.memoryValue}`
      : "-";

    html += `
      <tr class="${rowClass} ${markerClass}">
        <td class="rob-slot">ROB${slot}</td>
        <td>${formatROBMarkers(markers)}</td>
        <td>yes</td>
        <td>
          <div class="rob-tag">I${entry.instructionId}</div>
          <div>${escapeHtml(entry.rawText)}</div>
        </td>
        <td>
          <span class="rob-ready-state ${entry.ready ? "ready" : "pending"}">
            ${entry.ready ? "ready" : "pending"}
          </span>
        </td>
        <td>${destination}</td>
        <td>${entry.value}</td>
        <td>${memoryWrite}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  robTable.innerHTML = html;
}

function getDisplayROBEntries(cycle, entries) {
  const finalCommittedEntry = getFinalCommittedROBEntry(cycle, entries);

  if (!finalCommittedEntry) {
    return entries;
  }

  return entries.filter((entry) => {
    return entry.instructionId !== finalCommittedEntry.instructionId;
  });
}

function getDisplayRegisterProducers(cycle, producers, robEntries) {
  const finalCommittedEntry = getFinalCommittedROBEntry(cycle, robEntries);

  if (!finalCommittedEntry || !finalCommittedEntry.writesRegister) {
    return producers;
  }

  return producers.filter((producer) => {
    return producer.robTag !== finalCommittedEntry.robTag;
  });
}

function getFinalCommittedROBEntry(cycle, entries) {
  if (!isFinalSnapshot(currentIndex, trace)) {
    return null;
  }

  const committedInstructionId = getCommittedInstructionId(cycle);

  if (committedInstructionId === null) {
    return null;
  }

  return entries.find((entry) => {
    return entry.instructionId === committedInstructionId && entry.ready;
  }) || null;
}

function isFinalSnapshot(index, currentTrace) {
  return Boolean(currentTrace?.cycles) && index === currentTrace.cycles.length - 1;
}

function getCommittedInstructionId(snapshot) {
  const candidates = [];

  if (snapshot?.commitEvent) {
    candidates.push(snapshot.commitEvent);
  }

  if (Array.isArray(snapshot?.events)) {
    candidates.push(...snapshot.events);
  }

  for (const text of candidates) {
    const match = String(text).match(/\bCommitted I(\d+)\b/);

    if (match) {
      return Number(match[1]);
    }
  }

  return null;
}

function getROBCapacity(cycle = null) {
  if (typeof cycle?.rob?.capacity === "number" && cycle.rob.capacity > 0) {
    return cycle.rob.capacity;
  }

  if (typeof trace?.robCapacity === "number" && trace.robCapacity > 0) {
    return trace.robCapacity;
  }

  if (!trace || !trace.cycles) {
    return 4;
  }

  let maxTag = 0;

  for (const cycle of trace.cycles) {
    const entries = cycle.rob?.entries || [];

    for (const entry of entries) {
      maxTag = Math.max(maxTag, entry.robTag);
    }

    if (typeof cycle.rob?.head === "number") {
      maxTag = Math.max(maxTag, cycle.rob.head);
    }

    if (typeof cycle.rob?.tail === "number") {
      maxTag = Math.max(maxTag, cycle.rob.tail);
    }
  }

  return Math.max(4, maxTag + 1);
}

function getROBMarkers(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];

  if (getFinalCommittedROBEntry(cycle, cycle.rob?.entries || [])) {
    return "";
  }

  const markers = [];

  if (cycle.rob && cycle.rob.head === slot) {
    markers.push("HEAD");
  }

  if (cycle.rob && cycle.rob.tail === slot) {
    markers.push("TAIL");
  }

  return markers.join(" / ");
}

function formatROBMarkers(markerText) {
  if (!markerText) {
    return "-";
  }

  return markerText
    .split(" / ")
    .map((marker) => {
      const className = marker.toLowerCase();
      return `<span class="rob-marker ${className}">${marker}</span>`;
    })
    .join("");
}

function getROBMarkerClass(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];

  if (getFinalCommittedROBEntry(cycle, cycle.rob?.entries || [])) {
    return "";
  }

  let className = "";

  if (cycle.rob && cycle.rob.head === slot) {
    className += " head-row";
  }

  if (cycle.rob && cycle.rob.tail === slot) {
    className += " tail-row";
  }

  return className;
}

// Reservation station rendering
function renderReservationStations(cycle) {
  const activeEntries = cycle?.activeInstructions || [];
  const grouped = groupInstructionsByResource(activeEntries, {
    includeExecuting: false
  });
  const rsState = getRSState(cycle, grouped);

  renderRSTable(intRSTable, grouped.INT, rsState.INT.capacity, "INT", rsState.INT);
  renderRSTable(mulRSTable, grouped.MUL, rsState.MUL.capacity, "MUL", rsState.MUL);
  renderRSTable(fpAddRSTable, grouped.FP_ADD, rsState.FP_ADD.capacity, "FP_ADD", rsState.FP_ADD);
  renderRSTable(fpMulRSTable, grouped.FP_MUL, rsState.FP_MUL.capacity, "FP_MUL", rsState.FP_MUL);
  renderRSTable(loadBufferTable, grouped.LOAD, rsState.LOAD.capacity, "LOAD", rsState.LOAD);
  renderRSTable(storeBufferTable, grouped.STORE, rsState.STORE.capacity, "STORE", rsState.STORE);
}

function renderRSTable(container, entries, capacity, type, usage) {
  if (!container) return;

  const visibleCapacity = Math.max(capacity || 0, entries.length);
  const displayUsage = usage || {
    used: entries.length,
    capacity: visibleCapacity
  };

  let html = `
    <div class="rs-table-summary">
      ${escapeHtml(formatRSTableTitle(type))}: ${displayUsage.used}/${displayUsage.capacity} waiting
    </div>
    <table class="rs-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>Instruction</th>
          <th>Vj</th>
          <th>Vk</th>
          <th>Qj</th>
          <th>Qk</th>
          <th>ROB</th>
          <th>Remaining</th>
          <th>Status</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (let i = 0; i < visibleCapacity; i++) {
    const entry = entries[i];

    if (!entry) {
      html += `
        <tr class="empty-row">
          <td>${type}${i}</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td><span class="rs-status empty">empty</span></td>
        </tr>
      `;
      continue;
    }

    const rowClass = entry.executing ? "occupied-row executing-row" : "occupied-row waiting-row";
    const stateText = entry.executing ? "executing" : "waiting";
    const label = getInstructionClassLabel(entry.rawText);

    html += `
      <tr class="${rowClass}">
        <td>${type}${i}</td>
        <td>
          <span class="instruction-label ${label.className}">${escapeHtml(label.text)}</span>
          ${escapeHtml(entry.rawText)}
        </td>
        <td>${entry.vj}</td>
        <td>${entry.vk}</td>
        <td class="rs-tag">${formatTag(entry.qj)}</td>
        <td class="rs-tag">${formatTag(entry.qk)}</td>
        <td class="rs-tag">ROB${entry.robTag}</td>
        <td>${entry.remainingCycles}</td>
        <td>${escapeHtml(entry.waitingReason || stateText)}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  container.innerHTML = html;
}

function formatRSTableTitle(type) {
  if (type === "LOAD") return "Load Buffer";
  if (type === "STORE") return "Store Buffer";

  return `${type} RS`;
}

function inferRSType(rawText) {
  return getResourceType(rawText);
}

function groupInstructionsByResource(activeEntries, options = {}) {
  const includeExecuting = options.includeExecuting !== false;
  const grouped = {
    INT: [],
    MUL: [],
    FP_ADD: [],
    FP_MUL: [],
    LOAD: [],
    STORE: []
  };

  for (const entry of activeEntries || []) {
    if (!includeExecuting && !isReservationStationEntry(entry)) {
      continue;
    }

    const type = getResourceType(entry.rawText);

    if (grouped[type]) {
      grouped[type].push(entry);
    }
  }

  return grouped;
}

function isReservationStationEntry(entry) {
  if (!entry || entry.executing) {
    return false;
  }

  if (typeof entry.remainingCycles === "number" && entry.remainingCycles <= 0) {
    return false;
  }

  return true;
}

function getRSState(cycle, grouped) {
  return {
    INT: getRSUsage(cycle, "INT", grouped.INT, 2),
    MUL: getRSUsage(cycle, "MUL", grouped.MUL, 2),
    FP_ADD: getRSUsage(cycle, "FP_ADD", grouped.FP_ADD, 2),
    FP_MUL: getRSUsage(cycle, "FP_MUL", grouped.FP_MUL, 4),
    LOAD: getRSUsage(cycle, "LOAD", grouped.LOAD, 2),
    STORE: getRSUsage(cycle, "STORE", grouped.STORE, 2)
  };
}

function getRSUsage(cycle, key, entries, defaultCapacity) {
  const traced = cycle?.rsState?.[key];

  if (
    traced &&
    typeof traced.used === "number" &&
    typeof traced.capacity === "number"
  ) {
    return {
      used: traced.used,
      capacity: traced.capacity
    };
  }

  return {
    used: entries.length,
    capacity: defaultCapacity
  };
}

function getResourceType(rawText) {
  const op = getOpcode(rawText);

  if (op === "FADD" || op === "FSUB") {
    return "FP_ADD";
  }

  if (op === "FMUL" || op === "FDIV") {
    return "FP_MUL";
  }

  if (op === "MUL") {
    return "MUL";
  }

  if (op === "LD" || op === "LOAD") {
    return "LOAD";
  }

  if (op === "SD" || op === "STORE") {
    return "STORE";
  }

  return "INT";
}

function getRSBoxForResource(resource) {
  const boxes = {
    INT: intRsBox,
    MUL: mulRsBox,
    FP_ADD: fpAddRsBox,
    FP_MUL: fpMulRsBox,
    LOAD: loadBufferBox,
    STORE: storeBufferBox
  };

  return boxes[resource] || intRsBox;
}

function getFUBoxForResource(resource) {
  const boxes = {
    INT: intFuBox,
    MUL: mulFuBox,
    FP_ADD: fpAddFuBox,
    FP_MUL: fpMulFuBox,
    LOAD: memFuBox,
    STORE: memFuBox,
    MEM: memFuBox
  };

  return boxes[resource] || intFuBox;
}

function formatResourceLabel(resource) {
  const labels = {
    INT: "INT path",
    MUL: "MUL path",
    FP_ADD: "FP_ADD path",
    FP_MUL: "FP_MUL path",
    LOAD: "load path",
    STORE: "store path"
  };

  return labels[resource] || "instruction issue";
}

function getInstructionClassLabel(rawText) {
  const resource = getResourceType(rawText);
  const labels = {
    INT: { text: "INT", className: "int-instruction" },
    MUL: { text: "MUL", className: "mul-instruction" },
    FP_ADD: { text: "FP Add", className: "fp-add-instruction" },
    FP_MUL: { text: "FP Mul/Div", className: "fp-mul-instruction" },
    LOAD: { text: "Load", className: "mem-instruction" },
    STORE: { text: "Store", className: "mem-instruction" }
  };

  return labels[resource] || labels.INT;
}

function getFUState(cycle, grouped) {
  const pipelineSummary = getPipelineSummary(cycle);
  const config = getTraceArchitectureConfig();
  const fallback = {
    INT: { busy: countExecuting(grouped.INT), total: config.intFuCount },
    MUL: { busy: countExecuting(grouped.MUL), total: config.mulFuCount },
    FP_ADD: {
      busy: pipelineSummary.FP_ADD.used || countExecuting(grouped.FP_ADD),
      total: pipelineSummary.FP_ADD.total ||
        config.fpAddPipelineCount * config.fpAddPipelineDepth,
      pipelines: pipelineSummary.FP_ADD.pipelines || config.fpAddPipelineCount,
      pipelined: true
    },
    FP_MUL: {
      busy: pipelineSummary.FP_MUL.used || countExecuting(grouped.FP_MUL),
      total: pipelineSummary.FP_MUL.total ||
        config.fpMulPipelineCount * config.fpMulPipelineDepth,
      pipelines: pipelineSummary.FP_MUL.pipelines || config.fpMulPipelineCount,
      pipelined: true
    },
    MEM: {
      busy: countExecuting(grouped.LOAD) + countExecuting(grouped.STORE),
      total: config.memFuCount
    }
  };

  const traceFUState = cycle?.fuState;

  if (!traceFUState || typeof traceFUState !== "object") {
    return fallback;
  }

  for (const key of Object.keys(fallback)) {
    const value = traceFUState[key];
    const hasPipelineTrace = Boolean(fallback[key].pipelined);

    if (typeof value === "number") {
      if (!hasPipelineTrace) {
        fallback[key].busy = value;
      }
    } else if (value && typeof value === "object") {
      if (!hasPipelineTrace) {
        fallback[key].busy = value.busy ?? value.active ?? fallback[key].busy;
        fallback[key].total = value.total ?? value.capacity ?? fallback[key].total;
      }

      fallback[key].pipelines = value.pipelines ?? fallback[key].pipelines;
    }
  }

  return fallback;
}

function getPipelineSummary(cycle) {
  return {
    FP_ADD: summarizePipeline(cycle?.fuPipelines?.FP_ADD),
    FP_MUL: summarizePipeline(cycle?.fuPipelines?.FP_MUL)
  };
}

function summarizePipeline(pipelines) {
  if (!Array.isArray(pipelines)) {
    return { used: 0, total: 0, pipelines: 0 };
  }

  let used = 0;
  let total = 0;

  for (const pipeline of pipelines) {
    if (!Array.isArray(pipeline)) {
      continue;
    }

    total += pipeline.length;

    for (const stage of pipeline) {
      if (isPipelineStageOccupied(stage)) {
        used++;
      }
    }
  }

  return {
    used,
    total,
    pipelines: pipelines.length
  };
}

function isPipelineStageOccupied(stage) {
  if (stage && typeof stage === "object") {
    return Boolean(stage.occupied) || typeof stage.instructionId === "number";
  }

  if (typeof stage === "string") {
    return stage !== "--" && stage.trim() !== "";
  }

  return typeof stage === "number" && stage >= 0;
}

function countExecuting(entries) {
  return (entries || []).filter((entry) => entry.executing).length;
}

function formatFUUsage(state) {
  if (state?.pipelined) {
    return `${state.busy}/${state.total} pipeline slots used`;
  }

  return `${state.busy}/${state.total} busy`;
}

function formatFUUsageCompact(state) {
  if (state?.pipelined) {
    return `${state.busy}/${state.total} slots`;
  }

  return `${state.busy}/${state.total} busy`;
}

function formatFUPipelineCount(state) {
  if (!state?.pipelined) {
    return "";
  }

  return `${state.pipelines} ${state.pipelines === 1 ? "pipeline" : "pipelines"}`;
}

function renderFUState(cycle) {
  if (!fuStatePanel) return;

  const grouped = groupInstructionsByResource(cycle.activeInstructions || []);
  const fuState = getFUState(cycle, grouped);

  const rows = [
    {
      key: "INT",
      name: "Integer FU",
      type: "INT",
      state: fuState.INT,
      entries: grouped.INT,
      operations: "ADD, ADDI, SUB, BEQ, BNE"
    },
    {
      key: "MUL",
      name: "Multiply FU",
      type: "MUL",
      state: fuState.MUL,
      entries: grouped.MUL,
      operations: "MUL"
    },
    {
      key: "FP_ADD",
      name: "FP Add FU",
      type: "FP_ADD",
      state: fuState.FP_ADD,
      entries: grouped.FP_ADD,
      operations: "FADD, FSUB"
    },
    {
      key: "FP_MUL",
      name: "FP Mul/Div FU",
      type: "FP_MUL",
      state: fuState.FP_MUL,
      entries: grouped.FP_MUL,
      operations: "FMUL, FDIV"
    },
    {
      key: "MEM",
      name: "Memory FU",
      type: "MEM",
      state: fuState.MEM,
      entries: [...grouped.LOAD, ...grouped.STORE],
      operations: "LD, SD"
    }
  ];

  let html = `
    <table class="fu-state-table">
      <thead>
        <tr>
          <th>Functional Unit</th>
          <th>Type</th>
          <th>Busy</th>
          <th>Status</th>
          <th>Current Instruction</th>
          <th>Operations</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const row of rows) {
    const busy = Number(row.state.busy) || 0;
    const total = Number(row.state.total) || 0;
    const activeClass = busy > 0 ? "fu-busy-row" : "fu-free-row";
    const executing = row.entries.filter((entry) => entry.executing);
    const visibleEntries = executing.length > 0 ? executing : row.entries;
    const currentInstruction = visibleEntries.length > 0
      ? visibleEntries.map(formatFUInstruction).join("<br>")
      : "-";

    html += `
      <tr class="${activeClass}">
        <td><strong>${escapeHtml(row.name)}</strong></td>
        <td><span class="fu-type-pill ${escapeHtml(row.key.toLowerCase())}">${escapeHtml(row.type)}</span></td>
        <td>${escapeHtml(formatFUUsage(row.state))}</td>
        <td>${busy > 0 ? "busy" : "free"}</td>
        <td>${currentInstruction}</td>
        <td>
          ${escapeHtml(row.operations)}
          ${row.state.pipelined ? `<div class="fu-pipeline-note">${escapeHtml(formatFUPipelineCount(row.state))}</div>` : ""}
        </td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  fuStatePanel.innerHTML = html;
}

function formatFUInstruction(entry) {
  const label = getInstructionClassLabel(entry.rawText);
  const state = entry.executing ? "executing" : "waiting";
  const remaining = typeof entry.remainingCycles === "number"
    ? `, ${entry.remainingCycles} rem`
    : "";

  return `
    <span class="instruction-label ${label.className}">${escapeHtml(label.text)}</span>
    ${escapeHtml(entry.rawText || "-")}
    <span class="fu-instruction-state">(${state}${remaining})</span>
  `;
}

function renderFPPipelines(cycle) {
  if (!fpPipelinePanel) return;

  const pipelines = cycle?.fuPipelines;

  if (!pipelines || typeof pipelines !== "object") {
    fpPipelinePanel.innerHTML = '<div class="empty">No pipeline data in this trace.</div>';
    return;
  }

  const instructionMap = buildInstructionMap(cycle);
  const sections = [
    {
      title: "FP_ADD Pipeline",
      key: "FP_ADD",
      expectedStages: 3,
      pipelines: pipelines.FP_ADD
    },
    {
      title: "FP_MUL Pipeline",
      key: "FP_MUL",
      expectedStages: 5,
      pipelines: pipelines.FP_MUL
    }
  ];

  fpPipelinePanel.innerHTML = sections
    .map((section) => renderPipelineSection(section, instructionMap))
    .join("");
}

function buildInstructionMap(cycle) {
  const map = new Map();

  for (const entry of cycle?.activeInstructions || []) {
    if (typeof entry.instructionId === "number") {
      map.set(entry.instructionId, entry.rawText || "");
    }
  }

  for (const entry of trace?.instructionStatus || []) {
    if (typeof entry.instructionId === "number" && !map.has(entry.instructionId)) {
      map.set(entry.instructionId, entry.rawText || "");
    }
  }

  return map;
}

function renderPipelineSection(section, instructionMap) {
  const pipelines = Array.isArray(section.pipelines)
    ? section.pipelines
    : [];

  if (pipelines.length === 0) {
    return `
      <div class="pipeline-section">
        <h3>${escapeHtml(section.title)}</h3>
        <div class="empty">No ${escapeHtml(section.key)} pipeline data in this trace.</div>
      </div>
    `;
  }

  const maxStages = Math.max(
    section.expectedStages,
    ...pipelines.map((pipeline) => Array.isArray(pipeline) ? pipeline.length : 0)
  );
  const stageLabels = Array.from({ length: maxStages }, (_, index) => {
    return `<div class="pipeline-stage-label">Stage ${index}</div>`;
  }).join("");

  const rows = pipelines.map((pipeline, pipeIndex) => {
    const stages = Array.isArray(pipeline) ? pipeline : [];

    return `
      <div class="pipeline-row">
        <div class="pipeline-name">Pipe ${pipeIndex}</div>
        <div class="pipeline-stages">
          ${stages.map((stage) => renderPipelineStage(stage, instructionMap)).join("")}
        </div>
      </div>
    `;
  }).join("");

  return `
    <div class="pipeline-section">
      <h3>${escapeHtml(section.title)}</h3>
      <div class="pipeline-header">
        <div></div>
        <div class="pipeline-stage-labels">${stageLabels}</div>
      </div>
      ${rows}
    </div>
  `;
}

function renderPipelineStage(stage, instructionMap) {
  const instructionId = getPipelineInstructionId(stage);
  const occupied = instructionId >= 0;
  const text = occupied ? `I${instructionId}` : "--";
  const instructionText = occupied ? instructionMap.get(instructionId) || "" : "";
  const title = occupied
    ? `I${instructionId}${instructionText ? `: ${instructionText}` : ""}`
    : "empty";

  return `
    <div class="pipeline-stage ${occupied ? "occupied" : "empty-stage"}" title="${escapeHtml(title)}">
      ${escapeHtml(text)}
    </div>
  `;
}

function getPipelineInstructionId(stage) {
  if (stage && typeof stage === "object") {
    return typeof stage.instructionId === "number"
      ? stage.instructionId
      : -1;
  }

  if (typeof stage === "string") {
    const match = stage.match(/^I?(\d+)$/i);
    return match ? Number(match[1]) : -1;
  }

  return typeof stage === "number" ? stage : -1;
}

function renderPerformanceStats() {
  if (!performanceStatsPanel) return;

  const stats = trace?.performanceStats;

  if (!stats || typeof stats !== "object") {
    performanceStatsPanel.innerHTML = '<div class="empty">Performance statistics not available for this trace.</div>';
    return;
  }

  // Stall cycle counters answer "did this cycle stall at least once?";
  // stall event counters count each recorded cause, so they can be larger.
  performanceStatsPanel.innerHTML = `
    <div class="performance-stats-grid">
      ${renderStatItem("Total cycles", formatIntegerStat(stats.totalCycles))}
      ${renderStatItem("Committed instructions", formatIntegerStat(stats.committedInstructions))}
      ${renderStatItem("IPC", formatDecimalStat(stats.ipc, 2))}
      ${renderStatItem("Cycles with any stall", formatIntegerStat(stats.cyclesWithAnyStall))}
      ${renderStatItem("Issue stall cycles", formatIntegerStat(stats.issueStallCycles))}
      ${renderStatItem("Backend stall cycles", formatIntegerStat(stats.backendStallCycles))}
      ${renderStatItem("Total stall events", formatIntegerStat(stats.totalStallEvents))}
      ${renderStatItem("ROB full stall cycles", formatIntegerStat(stats.robFullStallCycles))}
      ${renderStatItem("RS full stall cycles", formatIntegerStat(stats.rsFullStallCycles))}
      ${renderStatItem("RAW dependency stall events", formatIntegerStat(stats.rawDependencyStallEvents))}
      ${renderStatItem("FU busy stall events", formatIntegerStat(stats.fuBusyStallEvents))}
      ${renderStatItem("Memory ordering stall events", formatIntegerStat(stats.memoryOrderingStallEvents))}
      ${renderStatItem("CDB broadcasts", formatIntegerStat(stats.cdbBroadcasts))}
      ${renderStatItem("Branch count", formatIntegerStat(stats.branchCount))}
      ${renderStatItem("Branch accuracy", formatPercentStat(stats.branchAccuracy, 1))}
      ${renderStatItem("Branch mispredictions", formatIntegerStat(stats.branchMispredictions))}
    </div>

    <div class="performance-subsections">
      <div>
        <h3>Instruction Mix</h3>
        ${renderCompactStats(stats.instructionMix, [
          ["INT", "int"],
          ["MUL", "mul"],
          ["FP_ADD", "fpAdd"],
          ["FP_MUL", "fpMul"],
          ["LOAD", "load"],
          ["STORE", "store"],
          ["BRANCH", "branch"]
        ])}
      </div>
      <div>
        <h3>Max Occupancy</h3>
        ${renderCompactStats(stats.maxOccupancy, [
          ["ROB", "rob"],
          ["INT RS", "intRs"],
          ["MUL RS", "mulRs"],
          ["FP_ADD RS", "fpAddRs"],
          ["FP_MUL RS", "fpMulRs"],
          ["Load Buffer", "loadBuffer"],
          ["Store Buffer", "storeBuffer"],
          ["FP_ADD pipeline", "fpAddPipeline"],
          ["FP_MUL pipeline", "fpMulPipeline"]
        ])}
      </div>
      ${renderL1DPerformanceSection(stats)}
    </div>
  `;
}

function renderStatItem(label, value) {
  return `
    <div class="performance-stat-item">
      <span>${escapeHtml(label)}</span>
      <strong>${escapeHtml(value)}</strong>
    </div>
  `;
}

function renderCompactStats(source, rows) {
  const values = source && typeof source === "object" ? source : {};

  return `
    <div class="compact-stat-list">
      ${rows.map(([label, key]) => `
        <div class="summary-row">
          <span>${escapeHtml(label)}</span>
          <strong>${formatIntegerStat(values[key])}</strong>
        </div>
      `).join("")}
    </div>
  `;
}

function renderL1DPerformanceSection(stats) {
  if (!stats || typeof stats !== "object") {
    return "";
  }

  if (!stats.l1dEnabled) {
    return `
      <div class="l1d-performance-section">
        <h3>L1 Data Cache</h3>
        <div class="empty">L1 Data Cache: disabled</div>
      </div>
    `;
  }

  return `
    <div class="l1d-performance-section">
      <h3>L1 Data Cache</h3>
      <div class="compact-stat-list">
        <div class="summary-row"><span>Accesses</span><strong>${formatIntegerStat(stats.l1dAccesses)}</strong></div>
        <div class="summary-row"><span>Hits</span><strong>${formatIntegerStat(stats.l1dHits)}</strong></div>
        <div class="summary-row"><span>Misses</span><strong>${formatIntegerStat(stats.l1dMisses)}</strong></div>
        <div class="summary-row"><span>Hit rate</span><strong>${formatPercentStat(stats.l1dHitRate, 1)}</strong></div>
        <div class="summary-row"><span>Miss rate</span><strong>${formatPercentStat(stats.l1dMissRate, 1)}</strong></div>
        <div class="summary-row"><span>Writebacks</span><strong>${formatIntegerStat(stats.l1dWritebacks)}</strong></div>
        <div class="summary-row"><span>Average access latency</span><strong>${formatDecimalStat(stats.l1dAverageAccessLatency, 2)}</strong></div>
        <div class="summary-row"><span>Memory stall cycles</span><strong>${formatIntegerStat(stats.memoryStallCycles)}</strong></div>
      </div>
    </div>
  `;
}

function formatIntegerStat(value) {
  return typeof value === "number" && Number.isFinite(value)
    ? String(Math.trunc(value))
    : "-";
}

function formatDecimalStat(value, digits = 2) {
  return typeof value === "number" && Number.isFinite(value)
    ? value.toFixed(digits)
    : "-";
}

function formatPercentStat(value, digits = 1) {
  return typeof value === "number" && Number.isFinite(value)
    ? `${value.toFixed(digits)}%`
    : "-";
}

function getOpcode(rawText) {
  const tokens = String(rawText)
    .toUpperCase()
    .match(/[A-Z]+/g) || [];
  const knownOpcodes = new Set([
    "FADD",
    "FSUB",
    "FMUL",
    "FDIV",
    "ADDI",
    "ADD",
    "SUB",
    "MUL",
    "BEQ",
    "BNE",
    "LD",
    "LOAD",
    "SD",
    "STORE"
  ]);

  return tokens.find((token) => knownOpcodes.has(token)) || "";
}

function isMemoryInstruction(rawText) {
  const op = getOpcode(rawText);
  return op === "LD" || op === "SD" || op === "LOAD" || op === "STORE";
}

// LSQ rendering
function renderLSQ(entries) {
  if (!lsqEntries) return;

  lsqEntries.innerHTML = "";

  if (entries.length === 0) {
    lsqEntries.appendChild(emptyMessage("LSQ is empty."));
    return;
  }

  let html = `
    <table class="lsq-table">
      <thead>
        <tr>
          <th>Entry</th>
          <th>Type</th>
          <th>Instruction</th>
          <th>ROB</th>
          <th>Address Ready</th>
          <th>Address</th>
          <th>Value Ready</th>
          <th>Value</th>
        </tr>
      </thead>
      <tbody>
  `;

  entries.forEach((entry, index) => {
    const isLoad = Boolean(entry.isLoad);
    const isStore = Boolean(entry.isStore);
    const type = isLoad ? "Load" : isStore ? "Store" : "Unknown";
    const rowClasses = [
      isLoad ? "load-row" : "",
      isStore ? "store-row" : "",
      entry.addressReady ? "address-ready-row" : "address-not-ready-row",
      entry.valueReady ? "value-ready-row" : ""
    ].filter(Boolean).join(" ");

    html += `
      <tr class="${rowClasses}">
        <td class="rs-tag">LSQ${index}</td>
        <td><span class="lsq-type ${type.toLowerCase()}">${type}</span></td>
        <td>${escapeHtml(entry.rawText || "-")}</td>
        <td class="rs-tag">${formatTag(entry.robTag)}</td>
        <td>${formatReady(entry.addressReady)}</td>
        <td>${entry.addressReady ? entry.address : "-"}</td>
        <td>${formatReady(entry.valueReady)}</td>
        <td>${entry.valueReady ? entry.value : "-"}</td>
      </tr>
    `;
  });

  html += `
      </tbody>
    </table>
  `;

  lsqEntries.innerHTML = html;
}

function renderL1DCacheState(cache) {
  if (!l1dCacheStatePanel) return;

  l1dCacheStatePanel.innerHTML = "";

  if (!cache || typeof cache !== "object") {
    l1dCacheStatePanel.appendChild(emptyMessage("L1 Data Cache state not available for this trace."));
    return;
  }

  if (!cache.enabled) {
    l1dCacheStatePanel.appendChild(emptyMessage("L1 Data Cache disabled."));
    return;
  }

  const sets = Array.isArray(cache.sets) ? cache.sets : [];
  const numSets = Number.isFinite(Number(cache.numSets))
    ? Number(cache.numSets)
    : sets.length;
  const lineSizeBytes = Number.isFinite(Number(cache.lineSizeBytes))
    ? Number(cache.lineSizeBytes)
    : null;

  let html = `
    <div class="cache-state-summary">
      L1D: ${formatIntegerStat(numSets)} sets, ${lineSizeBytes === null ? "-" : formatIntegerStat(lineSizeBytes)}-byte lines
    </div>
    <table class="cache-table">
      <thead>
        <tr>
          <th>Set</th>
          <th>Valid</th>
          <th>Dirty</th>
          <th>Tag</th>
          <th>Block contents</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const line of sets) {
    const valid = Boolean(line.valid);
    const dirty = Boolean(line.dirty);
    const tag = valid && line.tag !== null && line.tag !== undefined
      ? line.tag
      : "-";
    const blockContents = valid
      ? formatCacheBlockValues(line.blockValues)
      : "-";

    html += `
      <tr class="${dirty ? "dirty-cache-row" : ""}">
        <td class="rs-tag">${formatIntegerStat(Number(line.index))}</td>
        <td>${formatReady(valid)}</td>
        <td>${formatReady(dirty)}</td>
        <td>${escapeHtml(tag)}</td>
        <td><span class="cache-block-values">${escapeHtml(blockContents)}</span></td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  l1dCacheStatePanel.innerHTML = html;
}

function formatCacheBlockValues(blockValues) {
  if (!Array.isArray(blockValues) || blockValues.length === 0) {
    return "-";
  }

  return blockValues.map((entry) => {
    return `[${formatIntegerStat(Number(entry.address))}]=${formatIntegerStat(Number(entry.value))}`;
  }).join(", ");
}

// Register producer rendering
function renderRegisterProducers(producers) {
  if (!registerProducers) return;

  registerProducers.innerHTML = "";

  if (!Array.isArray(producers) || producers.length === 0) {
    registerProducers.appendChild(emptyMessage("No active register producers."));
    return;
  }

  const previousCycle = currentIndex > 0 ? trace.cycles[currentIndex - 1] : null;
  const previousProducers = new Map(
    (previousCycle?.registerProducers || [])
      .map((producer) => [getProducerRegister(producer), producer.robTag])
  );

  const sortedProducers = [...producers].sort(
    (left, right) => getProducerRegister(left) - getProducerRegister(right)
  );

  let html = `
    <table class="producer-table">
      <thead>
        <tr>
          <th>Register</th>
          <th>Producer ROB</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const producer of sortedProducers) {
    const registerNumber = getProducerRegister(producer);
    const changed = previousCycle
      ? previousProducers.get(registerNumber) !== producer.robTag
      : false;

    html += `
      <tr class="${changed ? "changed-row" : ""}">
        <td class="rs-tag">R${registerNumber}</td>
        <td class="rs-tag">${formatTag(producer.robTag)}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  registerProducers.innerHTML = html;
}

function getProducerRegister(producer) {
  return producer.register ?? producer.registerNumber ?? -1;
}

// Branch predictor rendering
function renderBranchPredictions(cycle) {
  const selectedPredictor = getSelectedPredictorType(cycle);

  renderPredictorOverview(selectedPredictor);
  renderPredictorState(cycle.predictorState, selectedPredictor);

  if (!branchPredictorSummary || !branchPredictorTable) return;

  branchPredictorSummary.innerHTML = "";
  branchPredictorTable.innerHTML = "";

  if (!Object.prototype.hasOwnProperty.call(cycle, "branchPredictions")) {
    branchPredictorSummary.appendChild(emptyMessage("Branch prediction data not available."));
    return;
  }

  const predictions = Array.isArray(cycle.branchPredictions)
    ? cycle.branchPredictions
    : [];

  renderBranchSummary(predictions, selectedPredictor);

  if (predictions.length === 0) {
    branchPredictorTable.appendChild(emptyMessage("No branch predictions issued yet."));
    return;
  }

  if (normalizePredictorType(selectedPredictor) === "gshare") {
    renderGShareBranchSummaryTable(predictions);
    return;
  }

  renderDefaultBranchSummaryTable(predictions, selectedPredictor);
}

function renderPredictorOverview(predictorType) {
  if (!branchPredictorOverview) return;

  const normalizedType = normalizePredictorType(predictorType);
  const details = PREDICTOR_DETAILS[normalizedType] || {
    name: predictorType || "-",
    type: "-",
    indexing: "-",
    state: "-"
  };

  branchPredictorOverview.innerHTML = `
    <div class="predictor-overview-card">
      <div class="predictor-overview-title">${escapeHtml(details.name)}</div>
      <div class="predictor-overview-grid">
        <div>
          <span>Type</span>
          <strong>${escapeHtml(details.type)}</strong>
        </div>
        <div>
          <span>Indexing</span>
          <strong>${escapeHtml(details.indexing)}</strong>
        </div>
        <div>
          <span>State</span>
          <strong>${escapeHtml(details.state)}</strong>
        </div>
      </div>
    </div>
  `;
}

function renderPredictorState(predictorState, selectedPredictor) {
  if (!predictorStateTable) return;

  predictorStateTable.innerHTML = "";

  const hasPredictorState = Boolean(predictorState);
  const predictorType = normalizePredictorType(
    predictorState?.predictorType || selectedPredictor
  );
  const entries = Array.isArray(predictorState?.entries)
    ? predictorState.entries
    : getFallbackPredictorStateEntries(predictorType);

  let ghrHtml = "";

  if (predictorType === "gshare") {
    const ghr = typeof predictorState?.globalHistory === "number"
      ? predictorState.globalHistory
      : -1;
    const bits = typeof predictorState?.globalHistoryBits === "number" &&
      predictorState.globalHistoryBits > 0
      ? predictorState.globalHistoryBits
      : undefined;
    const ghrText = ghr >= 0 ? `${formatBinary(ghr, bits)} (${ghr})` : "-";

    ghrHtml = `
      <div class="gshare-history">
        <span>Global History Register</span>
        <strong>${escapeHtml(ghrText)}</strong>
      </div>
    `;
  }

  if (entries.length === 0) {
    const message = hasPredictorState
      ? "No initialized predictor table entries yet."
      : "Predictor table state not available in this trace.";

    predictorStateTable.innerHTML = `
      ${ghrHtml}
      <div class="empty">${message}</div>
    `;
    return;
  }

  let html = `
    ${ghrHtml}
    <table class="predictor-state-table">
      <thead>
        <tr>
          <th>Index</th>
          <th>Predictor State</th>
          <th>Meaning</th>
          <th>Prediction</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const entry of entries) {
    const state = typeof entry.state === "number" ? entry.state : -1;
    const stateBits = entry.stateBits ||
      formatPredictorStateBits(predictorType, state);
    const meaning = entry.stateText ||
      predictorStateMeaning(predictorType, state);
    const prediction = entry.prediction ||
      predictorStatePrediction(predictorType, state);

    html += `
      <tr class="known-predictor-entry">
        <td class="rs-tag">${entry.index >= 0 ? entry.index : "-"}</td>
        <td class="rs-tag">${escapeHtml(stateBits || "-")}</td>
        <td>${escapeHtml(meaning || "-")}</td>
        <td>${escapeHtml(prediction || "-")}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  predictorStateTable.innerHTML = html;
}

function renderDefaultBranchSummaryTable(predictions, predictorType) {
  let html = `
    <table class="branch-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>PC</th>
          <th>Instruction</th>
          <th>State Before</th>
          <th>Predicted</th>
          <th>Actual</th>
          <th>State After</th>
          <th>Result</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const prediction of predictions) {
    const resolved = Boolean(prediction.branchResolved);
    const correct = resolved && Boolean(prediction.predictionCorrect);
    const rowClass = getBranchRowClass(resolved, correct);

    html += `
      <tr class="${rowClass}">
        <td class="rs-tag">${formatInstructionId(prediction)}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.pc)}</td>
        <td>${escapeHtml(prediction.instruction || "-")}</td>
        <td>${escapeHtml(formatBranchState(predictorType, prediction.stateBeforeText, prediction.stateBefore))}</td>
        <td>${formatDirection(prediction.predictedTaken)}</td>
        <td>${resolved ? formatDirection(prediction.actualTaken) : "pending"}</td>
        <td>${escapeHtml(resolved ? formatBranchState(predictorType, prediction.stateAfterText, prediction.stateAfter) : "pending")}</td>
        <td>${resolved ? (correct ? "Hit" : "Miss") : "pending"}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  branchPredictorTable.innerHTML = html;
}

function renderGShareBranchSummaryTable(predictions) {
  let html = `
    <table class="branch-table gshare-branch-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>PC</th>
          <th>Instruction</th>
          <th>GHR Before</th>
          <th>Index</th>
          <th>Counter Before</th>
          <th>Prediction</th>
          <th>Actual</th>
          <th>Counter After</th>
          <th>GHR After</th>
          <th>Result</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const prediction of predictions) {
    const resolved = Boolean(prediction.branchResolved);
    const correct = resolved && Boolean(prediction.predictionCorrect);
    const rowClass = getBranchRowClass(resolved, correct);

    html += `
      <tr class="${rowClass}">
        <td class="rs-tag">${formatInstructionId(prediction)}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.pc)}</td>
        <td>${escapeHtml(prediction.instruction || "-")}</td>
        <td>${escapeHtml(formatTraceGhr(prediction.globalHistoryBefore))}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.gshareIndex)}</td>
        <td>${escapeHtml(formatCounterSummary(prediction.counterBefore))}</td>
        <td>${formatDirection(prediction.predictedTaken)}</td>
        <td>${resolved ? formatDirection(prediction.actualTaken) : "pending"}</td>
        <td>${escapeHtml(resolved ? formatCounterSummary(prediction.counterAfter) : "pending")}</td>
        <td>${escapeHtml(resolved ? formatTraceGhr(prediction.globalHistoryAfter) : "-")}</td>
        <td>${resolved ? (correct ? "Hit" : "Miss") : "pending"}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  branchPredictorTable.innerHTML = html;
}

function renderBranchSummary(predictions, predictorType) {
  const resolved = predictions.filter((prediction) => prediction.branchResolved);
  const correct = resolved.filter((prediction) => prediction.predictionCorrect).length;
  const misses = resolved.length - correct;
  const accuracy = resolved.length > 0
    ? `${((100 * correct) / resolved.length).toFixed(1)}%`
    : "-";

  branchPredictorSummary.innerHTML = `
    <div class="summary-row">
      <span>Predictor</span>
      <strong>${escapeHtml(predictorType || "-")}</strong>
    </div>
    <div class="summary-row">
      <span>Resolved</span>
      <strong>${resolved.length}</strong>
    </div>
    <div class="summary-row">
      <span>Correct</span>
      <strong>${correct}</strong>
    </div>
    <div class="summary-row">
      <span>Mispredictions</span>
      <strong>${misses}</strong>
    </div>
    <div class="summary-row">
      <span>Accuracy</span>
      <strong>${accuracy}</strong>
    </div>
  `;
}

function renderAnalysisResults() {
  if (!analysisOverview || !analysisDetailTabs || !analysisDetails) return;

  analysisOverview.innerHTML = "";
  analysisDetailTabs.innerHTML = "";
  analysisDetails.innerHTML = "";

  const results = Array.isArray(analysisResults?.results)
    ? analysisResults.results
    : [];

  if (results.length === 0) {
    analysisOverview.appendChild(
      emptyMessage("Run prediction analysis to compare branch predictors.")
    );
    return;
  }

  const bestPredictor = analysisResults.bestPredictor;

  let overviewHtml = `
    <h3>Accuracy Overview</h3>
    <div class="table-scroll">
      <table class="accuracy-table performance-comparison-table">
        <thead>
          <tr>
            <th>Predictor</th>
            <th>Cycles</th>
            <th>Committed</th>
            <th>IPC</th>
            <th>Branch Accuracy</th>
            <th>Mispredictions</th>
            <th>Cycles With Stall</th>
            <th>Issue Stall Cycles</th>
            <th>Backend Stall Cycles</th>
            <th>ROB Stall Cycles</th>
            <th>RS Stall Cycles</th>
            <th>RAW Stall Events</th>
            <th>FU Stall Events</th>
            <th>Memory Stall Events</th>
            <th>Notes</th>
          </tr>
        </thead>
        <tbody>
  `;

  for (const result of results) {
    const isBest = result.predictor === bestPredictor && !result.error;
    const note = result.error
      ? result.error
      : getPredictorNote(result.predictor);
    const stats = result.performanceStats || {};
    const accuracy = result.error
      ? "-"
      : formatPercentStat(
          typeof stats.branchAccuracy === "number" ? stats.branchAccuracy : result.accuracy,
          1
        );
    const barWidth = result.error
      ? 0
      : Math.max(0, Math.min(100, Number(
          typeof stats.branchAccuracy === "number" ? stats.branchAccuracy : result.accuracy
        ) || 0));

    overviewHtml += `
      <tr class="${isBest ? "best-predictor-row" : ""}">
        <td><strong>${escapeHtml(getPredictorLabel(result.predictor))}</strong></td>
        <td>${result.error ? "-" : formatIntegerStat(stats.totalCycles)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.committedInstructions)}</td>
        <td>${result.error ? "-" : formatDecimalStat(stats.ipc, 2)}</td>
        <td>
          <div class="accuracy-value">${accuracy}</div>
          <div class="accuracy-bar"><span style="width: ${barWidth}%"></span></div>
        </td>
        <td>${result.error ? "-" : formatIntegerStat(stats.branchMispredictions)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.cyclesWithAnyStall)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.issueStallCycles)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.backendStallCycles)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.robFullStallCycles)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.rsFullStallCycles)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.rawDependencyStallEvents)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.fuBusyStallEvents)}</td>
        <td>${result.error ? "-" : formatIntegerStat(stats.memoryOrderingStallEvents)}</td>
        <td>${escapeHtml(note)}</td>
      </tr>
    `;
  }

  overviewHtml += `
        </tbody>
      </table>
    </div>
  `;

  analysisOverview.innerHTML = overviewHtml;

  let tabsHtml = "";

  for (const result of results) {
    const active = result.predictor === selectedAnalysisPredictor;

    tabsHtml += `
      <button
        type="button"
        class="analysis-detail-tab ${active ? "active" : ""}"
        data-predictor="${escapeHtml(result.predictor)}"
      >
        ${escapeHtml(getShortPredictorLabel(result.predictor))}
      </button>
    `;
  }

  analysisDetailTabs.innerHTML = tabsHtml;

  for (const button of analysisDetailTabs.querySelectorAll("button[data-predictor]")) {
    button.addEventListener("click", () => {
      selectedAnalysisPredictor = button.dataset.predictor;
      renderAnalysisResults();
    });
  }

  renderAnalysisDetails(results);
}

function renderAnalysisDetails(results) {
  if (!analysisDetails) return;

  const selectedResult = results.find((result) => {
    return result.predictor === selectedAnalysisPredictor;
  }) || results[0];

  if (!selectedResult) {
    analysisDetails.appendChild(emptyMessage("No predictor result selected."));
    return;
  }

  const predictorType = normalizePredictorType(selectedResult.predictor);

  if (selectedResult.error) {
    analysisDetails.innerHTML = `
      <div class="analysis-error">
        <strong>${escapeHtml(getPredictorLabel(selectedResult.predictor))}</strong>
        <span>${escapeHtml(selectedResult.error)}</span>
      </div>
    `;
    return;
  }

  const branchPredictions = Array.isArray(selectedResult.branchPredictions)
    ? selectedResult.branchPredictions
    : [];
  const selectedStats = selectedResult.performanceStats;

  analysisDetails.innerHTML = `
    <div class="analysis-accuracy-card">
      <div>
        <span>Predictor</span>
        <strong>${escapeHtml(getPredictorLabel(selectedResult.predictor))}</strong>
      </div>
      <div>
        <span>Correct</span>
        <strong>${selectedResult.correct}</strong>
      </div>
      <div>
        <span>Total</span>
        <strong>${selectedResult.total}</strong>
      </div>
      <div>
        <span>Accuracy</span>
        <strong>${formatAccuracy(selectedResult.correct, selectedResult.total)}</strong>
      </div>
    </div>

    <h3>Performance Statistics</h3>
    ${buildAnalysisPerformanceStats(selectedStats)}

    <h3>Branch Summary</h3>
    ${buildAnalysisBranchTable(branchPredictions, predictorType)}

    <h3>Predictor State Table</h3>
    ${buildAnalysisPredictorStateTable(selectedResult.predictorState, predictorType)}
  `;
}

function buildAnalysisPerformanceStats(stats) {
  if (!stats || typeof stats !== "object") {
    return '<div class="empty">Performance statistics not available for this trace.</div>';
  }

  return `
    <div class="performance-stats-grid analysis-performance-grid">
      ${renderStatItem("Cycles", formatIntegerStat(stats.totalCycles))}
      ${renderStatItem("Committed", formatIntegerStat(stats.committedInstructions))}
      ${renderStatItem("IPC", formatDecimalStat(stats.ipc, 2))}
      ${renderStatItem("Branch Accuracy", formatPercentStat(stats.branchAccuracy, 1))}
      ${renderStatItem("Mispredictions", formatIntegerStat(stats.branchMispredictions))}
      ${renderStatItem("Cycles With Stall", formatIntegerStat(stats.cyclesWithAnyStall))}
      ${renderStatItem("Issue Stall Cycles", formatIntegerStat(stats.issueStallCycles))}
      ${renderStatItem("Backend Stall Cycles", formatIntegerStat(stats.backendStallCycles))}
      ${renderStatItem("ROB Stall Cycles", formatIntegerStat(stats.robFullStallCycles))}
      ${renderStatItem("RS Stall Cycles", formatIntegerStat(stats.rsFullStallCycles))}
      ${renderStatItem("RAW Stall Events", formatIntegerStat(stats.rawDependencyStallEvents))}
      ${renderStatItem("FU Stall Events", formatIntegerStat(stats.fuBusyStallEvents))}
      ${renderStatItem("Memory Stall Events", formatIntegerStat(stats.memoryOrderingStallEvents))}
    </div>
    ${renderL1DPerformanceSection(stats)}
  `;
}

function buildAnalysisBranchTable(predictions, predictorType) {
  if (!Array.isArray(predictions) || predictions.length === 0) {
    return '<div class="empty">No branch predictions recorded.</div>';
  }

  if (normalizePredictorType(predictorType) === "gshare") {
    return buildAnalysisGShareBranchTable(predictions);
  }

  return buildAnalysisDefaultBranchTable(predictions, predictorType);
}

function buildAnalysisDefaultBranchTable(predictions, predictorType) {
  let html = `
    <div class="table-scroll">
      <table class="branch-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>PC</th>
            <th>Instruction</th>
            <th>State Before</th>
            <th>Predicted</th>
            <th>Actual</th>
            <th>State After</th>
            <th>Result</th>
          </tr>
        </thead>
        <tbody>
  `;

  for (const prediction of predictions) {
    const resolved = Boolean(prediction.branchResolved);
    const correct = resolved && Boolean(prediction.predictionCorrect);
    const rowClass = getBranchRowClass(resolved, correct);

    html += `
      <tr class="${rowClass}">
        <td class="rs-tag">${formatInstructionId(prediction)}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.pc)}</td>
        <td>${escapeHtml(prediction.instruction || "-")}</td>
        <td>${escapeHtml(formatBranchState(predictorType, prediction.stateBeforeText, prediction.stateBefore))}</td>
        <td>${formatDirection(prediction.predictedTaken)}</td>
        <td>${resolved ? formatDirection(prediction.actualTaken) : "pending"}</td>
        <td>${escapeHtml(resolved ? formatBranchState(predictorType, prediction.stateAfterText, prediction.stateAfter) : "pending")}</td>
        <td>${resolved ? (correct ? "Hit" : "Miss") : "pending"}</td>
      </tr>
    `;
  }

  html += `
        </tbody>
      </table>
    </div>
  `;

  return html;
}

function buildAnalysisGShareBranchTable(predictions) {
  let html = `
    <div class="table-scroll">
      <table class="branch-table gshare-branch-table">
        <thead>
          <tr>
            <th>ID</th>
            <th>PC</th>
            <th>Instruction</th>
            <th>GHR Before</th>
            <th>Index</th>
            <th>Counter Before</th>
            <th>Prediction</th>
            <th>Actual</th>
            <th>Counter After</th>
            <th>GHR After</th>
            <th>Result</th>
          </tr>
        </thead>
        <tbody>
  `;

  for (const prediction of predictions) {
    const resolved = Boolean(prediction.branchResolved);
    const correct = resolved && Boolean(prediction.predictionCorrect);
    const rowClass = getBranchRowClass(resolved, correct);

    html += `
      <tr class="${rowClass}">
        <td class="rs-tag">${formatInstructionId(prediction)}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.pc)}</td>
        <td>${escapeHtml(prediction.instruction || "-")}</td>
        <td>${escapeHtml(formatTraceGhr(prediction.globalHistoryBefore))}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.gshareIndex)}</td>
        <td>${escapeHtml(formatCounterSummary(prediction.counterBefore))}</td>
        <td>${formatDirection(prediction.predictedTaken)}</td>
        <td>${resolved ? formatDirection(prediction.actualTaken) : "pending"}</td>
        <td>${escapeHtml(resolved ? formatCounterSummary(prediction.counterAfter) : "pending")}</td>
        <td>${escapeHtml(resolved ? formatTraceGhr(prediction.globalHistoryAfter) : "-")}</td>
        <td>${resolved ? (correct ? "Hit" : "Miss") : "pending"}</td>
      </tr>
    `;
  }

  html += `
        </tbody>
      </table>
    </div>
  `;

  return html;
}

function buildAnalysisPredictorStateTable(predictorState, predictorType) {
  const normalizedType = normalizePredictorType(
    predictorState?.predictorType || predictorType
  );

  if (!predictorState) {
    const fallbackEntries = getFallbackPredictorStateEntries(normalizedType);

    if (fallbackEntries.length === 0) {
      return '<div class="empty">Predictor table state not available in this trace.</div>';
    }

    predictorState = {
      predictorType: normalizedType,
      entries: fallbackEntries
    };
  }

  const entries = Array.isArray(predictorState.entries)
    ? predictorState.entries
    : [];
  let ghrHtml = "";

  if (normalizedType === "gshare") {
    const ghr = typeof predictorState.globalHistory === "number"
      ? predictorState.globalHistory
      : -1;
    const bits = typeof predictorState.globalHistoryBits === "number" &&
      predictorState.globalHistoryBits > 0
      ? predictorState.globalHistoryBits
      : undefined;
    const ghrText = predictorState.globalHistoryText ||
      (ghr >= 0 ? formatBinary(ghr, bits) : "");

    ghrHtml = `
      <div class="gshare-history">
        <span>Global History Register</span>
        <strong>${escapeHtml(ghrText ? `${ghrText} (${ghr})` : "-")}</strong>
      </div>
    `;
  }

  if (entries.length === 0) {
    return `${ghrHtml}<div class="empty">No initialized predictor table entries yet.</div>`;
  }

  let html = `
    ${ghrHtml}
    <div class="table-scroll">
      <table class="predictor-state-table">
        <thead>
          <tr>
            <th>Index</th>
            <th>Predictor State</th>
            <th>Meaning</th>
            <th>Prediction</th>
          </tr>
        </thead>
        <tbody>
  `;

  for (const entry of entries) {
    const state = typeof entry.state === "number" ? entry.state : -1;
    const stateBits = entry.stateBits ||
      formatPredictorStateBits(normalizedType, state);
    const meaning = entry.stateText ||
      predictorStateMeaning(normalizedType, state);
    const prediction = entry.prediction ||
      predictorStatePrediction(normalizedType, state);

    html += `
      <tr class="known-predictor-entry">
        <td class="rs-tag">${entry.index >= 0 ? entry.index : "-"}</td>
        <td class="rs-tag">${escapeHtml(stateBits || "-")}</td>
        <td>${escapeHtml(meaning || "-")}</td>
        <td>${escapeHtml(prediction || "-")}</td>
      </tr>
    `;
  }

  html += `
        </tbody>
      </table>
    </div>
  `;

  return html;
}

function formatDirection(taken) {
  return taken ? "taken" : "not taken";
}

function formatAccuracy(correct, total) {
  if (typeof correct !== "number" || typeof total !== "number" || total <= 0) {
    return "-";
  }

  return `${((100 * correct) / total).toFixed(2)}%`;
}

function getPredictorLabel(predictor) {
  return PREDICTOR_DETAILS[normalizePredictorType(predictor)]?.name ||
    predictor ||
    "-";
}

function getShortPredictorLabel(predictor) {
  const normalizedType = normalizePredictorType(predictor);

  if (normalizedType === "always-not-taken") return "Always NT";
  if (normalizedType === "always-taken") return "Always T";

  return getPredictorLabel(normalizedType);
}

function getPredictorNote(predictor) {
  const normalizedType = normalizePredictorType(predictor);

  if (normalizedType === "always-not-taken") return "Static";
  if (normalizedType === "always-taken") return "Static";
  if (normalizedType === "one-bit") return "Local";
  if (normalizedType === "two-bit") return "Local saturating";
  if (normalizedType === "gshare") return "Global history";

  return "-";
}

function getSelectedPredictorType(cycle) {
  if (cycle?.predictorType) {
    return cycle.predictorType;
  }

  if (predictorSelect?.value) {
    return predictorSelect.value;
  }

  return "two-bit";
}

function normalizePredictorType(type) {
  const value = String(type || "").trim().toLowerCase();

  if (value === "not-taken") return "always-not-taken";
  if (value === "taken") return "always-taken";
  if (value === "1bit" || value === "1-bit") return "one-bit";
  if (value === "2bit" || value === "2-bit") return "two-bit";
  if (value === "g-share") return "gshare";

  return value;
}

function getFallbackPredictorStateEntries(predictorType) {
  if (predictorType === "always-not-taken") {
    return [{
      index: -1,
      state: -1,
      stateBits: "static",
      stateText: "Always Not Taken",
      prediction: "NT"
    }];
  }

  if (predictorType === "always-taken") {
    return [{
      index: -1,
      state: -1,
      stateBits: "static",
      stateText: "Always Taken",
      prediction: "T"
    }];
  }

  return [];
}

function formatOneBitState(state) {
  if (typeof state !== "number" || state < 0 || state > 1) {
    return "";
  }

  return state === 1 ? "1" : "0";
}

function formatTwoBitState(state) {
  if (typeof state !== "number" || state < 0 || state > 3) {
    return "";
  }

  return formatBinary(state, 2);
}

function predictorStateMeaning(type, state) {
  const predictorType = normalizePredictorType(type);

  if (predictorType === "always-not-taken") return "Always Not Taken";
  if (predictorType === "always-taken") return "Always Taken";

  if (typeof state !== "number" || state < 0) {
    return "N/A";
  }

  if (predictorType === "one-bit") {
    return state === 1 ? "Taken" : "Not Taken";
  }

  const meanings = [
    "Strongly Not Taken",
    "Weakly Not Taken",
    "Weakly Taken",
    "Strongly Taken"
  ];

  return meanings[state] || "N/A";
}

function predictorStatePrediction(type, state) {
  const predictorType = normalizePredictorType(type);

  if (predictorType === "always-not-taken") return "NT";
  if (predictorType === "always-taken") return "T";

  if (typeof state !== "number" || state < 0) {
    return "-";
  }

  if (predictorType === "one-bit") {
    return state === 1 ? "T" : "NT";
  }

  return state >= 2 ? "T" : "NT";
}

function formatBinary(value, bits) {
  if (typeof value !== "number" || value < 0) {
    return "";
  }

  const width = bits || Math.max(1, value.toString(2).length);
  return value.toString(2).padStart(width, "0");
}

function formatPredictorStateBits(type, state) {
  const predictorType = normalizePredictorType(type);

  if (predictorType === "always-not-taken" || predictorType === "always-taken") {
    return "static";
  }

  if (predictorType === "one-bit") {
    return formatOneBitState(state);
  }

  return formatTwoBitState(state);
}

function formatBranchState(type, text, state) {
  const predictorType = normalizePredictorType(type);
  const bits = formatPredictorStateBits(predictorType, state);
  const meaning = text && text !== "N/A"
    ? text
    : predictorStateMeaning(predictorType, state);

  if (!bits) {
    return meaning || "N/A";
  }

  return `${bits} ${meaning}`;
}

function formatCounterSummary(state) {
  if (typeof state !== "number" || state < 0) {
    return "N/A";
  }

  return `${formatTwoBitState(state)} ${predictorStateMeaning("gshare", state)}`;
}

function formatTraceGhr(value) {
  if (typeof value !== "number" || value < 0) {
    return "-";
  }

  return `${formatBinary(value, 4)} (${value})`;
}

function formatInstructionId(prediction) {
  return typeof prediction.instructionId === "number" &&
    prediction.instructionId >= 0
    ? `I${prediction.instructionId}`
    : "-";
}

function getBranchRowClass(resolved, correct) {
  if (!resolved) return "branch-pending-row";
  return correct ? "branch-correct-row" : "branch-miss-row";
}

function formatStateText(text, state) {
  if (text) {
    return state >= 0 ? `${state} ${text}` : text;
  }

  return state >= 0 ? String(state) : "N/A";
}

function formatNullableNumber(value) {
  return typeof value === "number" && value >= 0 ? value : "-";
}

// Register and memory rendering
function renderRegisterState(registers) {
  renderStateTable(registerState, registers, "R", "Register state not available");
}

function renderMemoryState(memory) {
  renderStateTable(memoryState, memory, "Mem", "Memory state not available");
}

function renderStateTable(container, values, labelPrefix, unavailableText) {
  if (!container) return;

  container.innerHTML = "";

  if (!Array.isArray(values)) {
    container.appendChild(emptyMessage(unavailableText));
    return;
  }

  const previousCycle = currentIndex > 0 ? trace.cycles[currentIndex - 1] : null;
  const previousValues = labelPrefix === "R"
    ? previousCycle?.registers
    : previousCycle?.memory;
  const displayValues = values.slice(0, 32);

  while (displayValues.length < 32) {
    displayValues.push(0);
  }

  const perRow = 4;
  let html = '<table class="state-table"><tbody>';

  for (let rowStart = 0; rowStart < 32; rowStart += perRow) {
    html += "<tr>";

    for (let offset = 0; offset < perRow; offset++) {
      const index = rowStart + offset;
      const value = displayValues[index];
      const changed = Array.isArray(previousValues) && previousValues[index] !== value;
      const label = labelPrefix === "R" ? `R${index}` : `Mem[${index}]`;

      html += `
        <th>${label}</th>
        <td class="${changed ? "changed-cell" : ""}">${value}</td>
      `;
    }

    html += "</tr>";
  }

  html += "</tbody></table>";
  container.innerHTML = html;
}

// Instruction status timeline rendering
function renderInstructionStatus(cycle) {
  if (!instructionStatusTable) return;

  instructionStatusTable.innerHTML = "";

  if (!Array.isArray(trace.instructionStatus)) {
    instructionStatusTable.appendChild(
      emptyMessage("Instruction status data not available.")
    );
    return;
  }

  const currentCycle = typeof cycle.cycle === "number"
    ? cycle.cycle
    : currentIndex + 1;

  const visibleEntries = trace.instructionStatus.filter((entry) => {
    const issueCycle = getStatusCycle(entry, "issueCycle");
    return issueCycle >= 0 && issueCycle <= currentCycle;
  });

  if (visibleEntries.length === 0) {
    instructionStatusTable.appendChild(emptyMessage("No instructions issued yet."));
    return;
  }

  let html = `
    <table class="status-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>PC</th>
          <th>Instruction</th>
          <th>Issue</th>
          <th>Exec Start</th>
          <th>Exec End</th>
          <th>Write CDB</th>
          <th>Commit</th>
          <th>Flush</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const entry of visibleEntries) {
    const committed = isStageVisible(getStatusCycle(entry, "commitCycle"), currentCycle);
    const flushed = isFlushVisible(entry, currentCycle);
    const currentRow = hasCurrentInstructionStatusEvent(entry, currentCycle);
    const rowClasses = [
      committed ? "status-committed-row" : "",
      flushed ? "status-flushed-row" : "",
      currentRow ? "status-current-row" : ""
    ].filter(Boolean).join(" ");

    html += `
      <tr class="${rowClasses}">
        <td class="rs-tag">I${entry.instructionId}</td>
        <td class="rs-tag">${formatNullableNumber(entry.pc)}</td>
        <td>${escapeHtml(entry.rawText || "-")}</td>
        ${renderStatusCycleCell(entry, "issueCycle", currentCycle, "issue-stage-cell")}
        ${renderStatusCycleCell(entry, "execStartCycle", currentCycle, "exec-stage-cell")}
        ${renderStatusCycleCell(entry, "execEndCycle", currentCycle, "exec-stage-cell")}
        ${renderStatusCycleCell(entry, "writebackCycle", currentCycle, "wb-stage-cell")}
        ${renderStatusCycleCell(entry, "commitCycle", currentCycle, "commit-stage-cell")}
        ${renderFlushStatusCell(entry, currentCycle)}
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  instructionStatusTable.innerHTML = html;
}

function renderStatusCycleCell(entry, fieldName, currentCycle, stageClass) {
  const cycle = getStatusCycle(entry, fieldName);

  if (cycle < 0) {
    return '<td class="status-missing-cell">-</td>';
  }

  if (cycle > currentCycle) {
    return `<td class="status-pending-cell ${stageClass}"></td>`;
  }

  const currentClass = cycle === currentCycle ? "current-stage-cell" : "";
  return `<td class="${stageClass} ${currentClass}">${cycle}</td>`;
}

function renderFlushStatusCell(entry, currentCycle) {
  if (!entry.flushed) {
    const commitCycle = getStatusCycle(entry, "commitCycle");
    const content = commitCycle >= 0 && commitCycle <= currentCycle ? "no" : "";
    const cellClass = content ? "status-no-flush-cell" : "status-pending-cell";
    return `<td class="${cellClass}">${content}</td>`;
  }

  const flushCycle = getStatusCycle(entry, "flushCycle");

  if (flushCycle >= 0 && flushCycle > currentCycle) {
    return '<td class="flush-stage-cell status-pending-cell"></td>';
  }

  const currentClass = flushCycle === currentCycle ? "current-stage-cell" : "";
  return `<td class="flush-stage-cell ${currentClass}">yes</td>`;
}

function getStatusCycle(entry, fieldName) {
  const value = entry[fieldName];
  return typeof value === "number" ? value : -1;
}

function isStageVisible(cycle, currentCycle) {
  return cycle >= 0 && cycle <= currentCycle;
}

function isFlushVisible(entry, currentCycle) {
  if (!entry.flushed) return false;

  const flushCycle = getStatusCycle(entry, "flushCycle");
  return flushCycle < 0 || flushCycle <= currentCycle;
}

function hasCurrentInstructionStatusEvent(entry, currentCycle) {
  const timingFields = [
    "issueCycle",
    "execStartCycle",
    "execEndCycle",
    "writebackCycle",
    "commitCycle",
    "flushCycle"
  ];

  return timingFields.some((fieldName) => {
    return getStatusCycle(entry, fieldName) === currentCycle;
  });
}

// Helper functions
function emptyMessage(text) {
  const div = document.createElement("div");
  div.className = "empty";
  div.textContent = text;
  return div;
}

function formatTag(tag) {
  return tag >= 0 ? `ROB${tag}` : "-";
}

function formatReady(isReady) {
  return `<span class="ready-pill ${isReady ? "ready" : "not-ready"}">${isReady ? "yes" : "no"}</span>`;
}

function hasEvent(events, text) {
  return events.some((event) => event.includes(text));
}

function getFirstEvent(events, text) {
  return events.find((event) => event.includes(text)) || "";
}

function getEventClass(event) {
  if (event.includes("Issue stalled")) return "event-stall";
  if (event.includes("CDB Broadcast")) return "event-cdb";
  if (event.includes("Committed")) return "event-commit";
  if (event.includes("Flushed")) return "event-flush";
  if (event.includes("misprediction")) return "event-mispredict";
  if (event.includes("Execution")) return "event-execute";
  if (event.includes("Branch")) return "event-branch";
  return "";
}

function getProgramLine(pc) {
  if (programLines.length === 0) {
    return "load .asm file";
  }

  if (pc < 0 || pc >= programLines.length) {
    return "program complete";
  }

  return programLines[pc];
}

function normalizeInstruction(text) {
  return String(text)
    .replace(/\/\/.*$/g, "")
    .replace(/#.*$/g, "")
    .replace(/\s+/g, " ")
    .trim()
    .toUpperCase();
}

function setText(element, value) {
  if (element) {
    element.textContent = value;
  }
}

function setRunStatus(message, state) {
  if (!runStatus) return;

  runStatus.textContent = message;
  runStatus.classList.remove("success", "error");

  if (state) {
    runStatus.classList.add(state);
  }
}

function setRunButtonDisabled(disabled) {
  if (runSimulationBtn) {
    runSimulationBtn.disabled = disabled;
  }
}

function isEditableTarget(target) {
  if (!target) return false;

  const tagName = target.tagName ? target.tagName.toLowerCase() : "";
  return tagName === "input" || tagName === "textarea" || target.isContentEditable;
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

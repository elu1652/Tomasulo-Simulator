let trace = null;
let programLines = [];
let currentIndex = 0;
let playTimer = null;

const traceFileInput = document.getElementById("traceFile");
const programFileInput = document.getElementById("programFile");

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
const programListing = document.getElementById("programListing");

const intRSTable = document.getElementById("intRSTable");
const mulRSTable = document.getElementById("mulRSTable");
const loadBufferTable = document.getElementById("loadBufferTable");
const storeBufferTable = document.getElementById("storeBufferTable");

const programBox = document.getElementById("programBox");
const issueBox = document.getElementById("issueBox");
const rsBox = document.getElementById("rsBox");
const fuBox = document.getElementById("fuBox");
const cdbBox = document.getElementById("cdbBox");
const robBox = document.getElementById("robBox");
const lsqBox = document.getElementById("lsqBox");
const memoryBox = document.getElementById("memoryBox");
const commitBox = document.getElementById("commitBox");

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
  commitBox
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

if (traceFileInput) {
  traceFileInput.addEventListener("change", handleTraceFile);
}

if (programFileInput) {
  programFileInput.addEventListener("change", handleProgramFile);
}

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

function handleTraceFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    try {
      const parsedTrace = JSON.parse(e.target.result);

      if (!parsedTrace.cycles || !Array.isArray(parsedTrace.cycles)) {
        alert("Invalid trace file: expected a cycles array.");
        return;
      }

      trace = parsedTrace;

      if (Array.isArray(trace.program)) {
        programLines = trace.program;
      }

      currentIndex = 0;

      if (cycleSlider) {
        cycleSlider.min = 0;
        cycleSlider.max = trace.cycles.length - 1;
        cycleSlider.value = 0;
      }

      render();
    } catch (error) {
      alert("Could not load trace file. Check the browser console for details.");
      console.error(error);
    }
  };

  reader.readAsText(file);
}

function handleProgramFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    programLines = parseProgramLines(e.target.result);
    render();
  };

  reader.readAsText(file);
}

function parseProgramLines(text) {
  return text
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0)
    .filter((line) => !line.startsWith("#"))
    .filter((line) => !line.startsWith("//"))
    .filter((line) => !line.endsWith(":"));
}

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

function render() {
  if (!trace) return;

  const cycle = trace.cycles[currentIndex];
  const events = cycle.events || [];

  if (cycleSlider) {
    cycleSlider.value = currentIndex;
  }

  setText(cycleText, `Cycle ${cycle.cycle}`);
  setText(pcText, `PC: ${cycle.pc}`);

  setText(issuedText, cycle.issuedInstruction || "none");
  setText(cdbText, cycle.cdbBroadcast || "none");
  setText(commitText, cycle.commitEvent || "none");

  setText(robHeadText, `ROB${cycle.rob.head}`);
  setText(robTailText, `ROB${cycle.rob.tail}`);
  setText(robCountText, cycle.rob.count);

  renderProgram(cycle);
  renderDatapath(cycle, events);
  renderEvents(events);
  renderROB(cycle.rob.entries || []);
  renderReservationStations(cycle.activeInstructions || []);
  renderLSQ(cycle.lsq || []);
}

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

  const issuedIndex = findIssuedProgramIndex(cycle.issuedInstruction);

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
  const hasLSQ = hasEvent(events, "LSQ");
  const hasMemory = hasEvent(events, "Memory Commit") || hasEvent(events, "Store result ready");

  const activeCount = cycle.activeInstructions ? cycle.activeInstructions.length : 0;
  const robCount = cycle.rob ? cycle.rob.count : 0;
  const lsqCount = cycle.lsq ? cycle.lsq.length : 0;

  setText(programBoxMain, `PC: ${cycle.pc}`);
  setText(programBoxSub, getProgramLine(cycle.pc));

  setText(issueBoxMain, cycle.issuedInstruction || "none");
  setText(issueBoxSub, hasStall ? "issue stalled" : "instruction issue");

  setText(rsBoxMain, `${activeCount} active`);

  setText(
    fuBoxMain,
    hasExecutionStart
      ? getFirstEvent(events, "Execution started")
      : "INT / MUL / MEM"
  );

  setText(
    fuBoxSub,
    hasExecutionComplete ? "completed this cycle" : "execution"
  );

  setText(cdbBoxMain, hasCDB ? cycle.cdbBroadcast : "none");

  setText(robBoxMain, `${robCount} entries`);
  setText(robBoxSub, `head ROB${cycle.rob.head}, tail ROB${cycle.rob.tail}`);

  setText(lsqBoxMain, `${lsqCount} entries`);
  setText(memoryBoxMain, hasMemory ? "memory event" : "loads/stores");
  setText(commitBoxMain, cycle.commitEvent || "none");

  if (hasIssued) {
    activate(programBox, arrowProgramIssue, issueBox, arrowIssueRS, rsBox);
  }

  if (hasStall) {
    markStall(issueBox, arrowProgramIssue);
  }

  if (hasExecutionStart || hasExecutionComplete) {
    activate(rsBox, arrowRSFU, fuBox);
  }

  if (hasCDB) {
    activate(fuBox, arrowFUCDB, cdbBox, arrowCDBROB, robBox);
  }

  if (hasCommit) {
    markCommit(robBox, arrowROBCommit, commitBox);
  }

  if (hasBranch) {
    activate(programBox, issueBox);
  }

  if (hasMispredict || hasFlush) {
    markFlush(programBox, issueBox, rsBox, robBox);
  }

  if (hasLSQ) {
    activate(rsBox, arrowRSLSQ, lsqBox);
  }

  if (hasMemory) {
    activate(lsqBox, arrowLSQMemory, memoryBox);
  }
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

function renderROB(entries) {
  if (!robTable) {
    console.error("Missing ROB container. Expected id='robTable' or id='robEntries'.");
    return;
  }

  const capacity = getROBCapacity();

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
          <td>${markers || "-"}</td>
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
        <td>${markers || "-"}</td>
        <td>yes</td>
        <td>
          <div class="rob-tag">I${entry.instructionId}</div>
          <div>${escapeHtml(entry.rawText)}</div>
        </td>
        <td>${entry.ready ? "yes" : "no"}</td>
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

function getROBCapacity() {
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

  return maxTag + 1;
}

function getROBMarkers(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];
  const markers = [];

  if (cycle.rob.head === slot) {
    markers.push("HEAD");
  }

  if (cycle.rob.tail === slot) {
    markers.push("TAIL");
  }

  return markers.join(" / ");
}

function getROBMarkerClass(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];
  let className = "";

  if (cycle.rob.head === slot) {
    className += " head-row";
  }

  if (cycle.rob.tail === slot) {
    className += " tail-row";
  }

  return className;
}

function renderReservationStations(activeEntries) {
  if (!intRSTable || !mulRSTable || !loadBufferTable || !storeBufferTable) {
    return;
  }

  const grouped = {
    INT: [],
    MUL: [],
    LOAD: [],
    STORE: []
  };

  for (const entry of activeEntries) {
    const type = inferRSType(entry.rawText);
    grouped[type].push(entry);
  }

  renderRSTable(intRSTable, grouped.INT, 2, "INT");
  renderRSTable(mulRSTable, grouped.MUL, 2, "MUL");
  renderRSTable(loadBufferTable, grouped.LOAD, 2, "LOAD");
  renderRSTable(storeBufferTable, grouped.STORE, 2, "STORE");
}

function renderRSTable(container, entries, capacity, type) {
  let html = `
    <table class="rs-table">
      <thead>
        <tr>
          <th>Slot</th>
          <th>Busy</th>
          <th>Instruction</th>
          <th>Op</th>
          <th>Vj</th>
          <th>Vk</th>
          <th>Qj</th>
          <th>Qk</th>
          <th>ROB</th>
          <th>Rem</th>
          <th>State</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (let i = 0; i < capacity; i++) {
    const entry = entries[i];

    if (!entry) {
      html += `
        <tr class="empty-row">
          <td>${type}${i}</td>
          <td>no</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>empty</td>
        </tr>
      `;
      continue;
    }

    const rowClass = entry.executing ? "executing-row" : "waiting-row";
    const stateText = entry.executing ? "executing" : "waiting";
    const op = getOpcode(entry.rawText);

    html += `
      <tr class="${rowClass}">
        <td>${type}${i}</td>
        <td>yes</td>
        <td>${escapeHtml(entry.rawText)}</td>
        <td class="rs-op">${escapeHtml(op)}</td>
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

function inferRSType(rawText) {
  const op = getOpcode(rawText);

  if (op === "MUL") {
    return "MUL";
  }

  if (op === "LD") {
    return "LOAD";
  }

  if (op === "SD") {
    return "STORE";
  }

  return "INT";
}

function getOpcode(rawText) {
  return String(rawText).trim().split(/\s+/)[0].toUpperCase();
}

function renderLSQ(entries) {
  if (!lsqEntries) return;

  lsqEntries.innerHTML = "";

  if (entries.length === 0) {
    lsqEntries.appendChild(emptyMessage("LSQ is empty."));
    return;
  }

  for (const entry of entries) {
    const card = document.createElement("div");
    card.className = "card";

    const type = entry.isLoad ? "Load" : "Store";

    card.innerHTML = `
      <div class="card-title">I${entry.instructionId} | ROB${entry.robTag}</div>
      <div>${escapeHtml(entry.rawText)}</div>
      <div class="card-row">
        <span>Type</span>
        <span>${type}</span>
      </div>
      <div class="card-row">
        <span>Address</span>
        <span>${entry.addressReady ? entry.address : "not ready"}</span>
      </div>
      <div class="card-row">
        <span>Value</span>
        <span>${entry.valueReady ? entry.value : "not ready"}</span>
      </div>
    `;

    lsqEntries.appendChild(card);
  }
}

function emptyMessage(text) {
  const div = document.createElement("div");
  div.className = "empty";
  div.textContent = text;
  return div;
}

function formatTag(tag) {
  return tag >= 0 ? `ROB${tag}` : "-";
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

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
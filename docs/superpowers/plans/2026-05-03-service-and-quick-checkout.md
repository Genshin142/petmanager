# Service Management & Quick Order Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform PetManager's service tracking into a professional Master-Detail management system and enable cross-module "Quick Checkout" functionality.

**Architecture:** A Master-Detail UI for service management using a split layout with a searchable list (Master) and a card-based detail view (Detail). A unified Quick Order dialog handles hybrid search across products and services.

**Tech Stack:** Qt (C++), QSS (Qt Style Sheets) for pixel-perfect design.

---

### Task 1: Service Data Layer Refinement

**Files:**
- Modify: `modules/servicedatamanager.cpp`
- Test: Create `tests/test_servicedatamanager.cpp`

- [ ] **Step 1: Implement basic CRUD and Mock data**
Ensure `ServiceDataManager` correctly handles the `ServiceInfo` struct with commission fields and populates mock data (Bath, Grooming, SPA).

- [ ] **Step 2: Commit**

---

### Task 2: Service Management UI - Master-Detail Skeleton

**Files:**
- Create: `modules/servicemanagementmodule.h`, `modules/servicemanagementmodule.cpp`
- Modify: `mainwindow.cpp` (to register the new module)

- [ ] **Step 1: Create Splitter Layout**
Implement a `QSplitter` with a left `QWidget` (Master) and right `QScrollArea` (Detail).

- [ ] **Step 2: Create Custom List Item Widget**
Create a widget for the list item matching the HTML mockup (Name, Subtext, Price).

- [ ] **Step 3: Commit**

---

### Task 3: Search Bar & Category Chips

**Files:**
- Modify: `modules/servicemanagementmodule.cpp`

- [ ] **Step 1: Implement Search Bar**
Add a `QLineEdit` with a search icon and connect its `textChanged` to filter the list.

- [ ] **Step 2: Implement Category Chips**
Create a horizontal layout of `QPushButton`s. Apply QSS to make them look like rounded chips.

- [ ] **Step 3: Implement Filtering Logic**
Connect chip clicks to filter the `ServiceDataManager::allServices()` results.

- [ ] **Step 4: Commit**

---

### Task 4: Detail View - Stats & Commission Cards (QSS focus)

**Files:**
- Modify: `modules/servicemanagementmodule.cpp`
- Create: `res/qss/service_detail.qss`

- [ ] **Step 1: Implement Performance Stats Cards**
Create custom `QFrame`s for "Total Bookings" and "Monthly Revenue" with large fonts and trend indicators.

- [ ] **Step 2: Implement Commission Config Card**
Create a form-like section for editing commission values.

- [ ] **Step 3: Apply Pixel-Perfect QSS**
Implement the specific rounded corners, borders, and fonts as per the HTML mockup.

- [ ] **Step 4: Commit**

---

### Task 5: Quick Order Dialog - Hybrid Search

**Files:**
- Create: `modules/quickorderdialog.h`, `modules/quickorderdialog.cpp`

- [ ] **Step 1: Implement Hybrid Search Logic**
Query both `ProductDataManager` and `ServiceDataManager`. Merge results and show them in a dropdown/list with `[SERV]` and `[PROD]` tags.

- [ ] **Step 2: Implement Checkout Logic**
Generate a single `OrderInfo` when "Settle" is clicked, updating sales counts and inventory.

- [ ] **Step 3: Commit**

---

### Task 6: Final Verification & Integration

- [ ] **Step 1: Manual Verification**
Verify that selecting a service in the list updates the detail panel instantly. Verify search and category filters work.

- [ ] **Step 2: Final Commit**

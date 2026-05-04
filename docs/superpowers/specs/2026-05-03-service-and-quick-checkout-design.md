# Service Management & Quick Order Design Spec

## Goal Description
Transform PetManager's service tracking into a professional management system. This involves creating a dedicated Service module, a cross-table Quick Checkout tool, and ensuring deep synchronization across inventory, appointments, and financial records.

## User Review Required
> [!IMPORTANT]
> **Data Integrity**: Service prices and names are snapshotted into orders at the moment of creation. Changing a service's profile will not affect historical financial records.
> [!NOTE]
> **Staff Commission**: The system will support both 'Fixed Amount' and 'Percentage' commissions to accommodate different store policies.

## Proposed Components

### 1. Data Layer (`common_types.h` & `ServiceDataManager`)
- **[NEW] `ServiceInfo` Struct**:
  - `id`: Unique identifier (e.g., SRV-1001)
  - `name`, `category`, `price`, `durationMinutes`
  - `commissionType`: "Fixed" | "Percentage"
  - `commissionValue`: double
  - `salesCount`: int
  - `isActive`: bool
- **[NEW] `ServiceDataManager`**: Singleton class handling CRUD for `ServiceInfo`. Includes mock data initialization for common pet services (Bath, Grooming, SPA).

### 2. Service Management Module (`ServiceManagementModule`)
- **Layout**: Master-Detail split view.
  - **Left Sidebar (Master)**:
    - **Search Bar**: QLineEdit with a search icon for filtering services by name/category.
    - **Category Filters**: A horizontal row of clickable tags (Chips) like "All", "Grooming", "Medical", etc.
    - **Service List**: A QListView or custom QWidget list showing:
      - Service Name (Bold)
      - Category & Duration (Subtext)
      - Price (Right-aligned)
  - **Right Panel (Detail)**:
    - **Performance Metrics**: Visual cards for "Total Sales" and "Monthly Revenue".
    - **Commission Config**: Dedicated section to view/edit fixed and percentage commissions.
    - **Usage History**: A list of recent service records and modification logs.
    - **Status Toggle**: A clear indicator if the service is "Active" or "Inactive".

### 3. Quick Checkout Dialog (`QuickOrderDialog`)
- **Member Search**: Integrated QLineEdit with auto-completion from `PetDataManager`.
- **Hybrid Search Bar**: 
  - Triggers search in both `ProductDataManager` and `ServiceDataManager`.
  - **Visual Tags**: Results prefixed with `[SERV]` (Blue) or `[PROD]` (Red, with stock count).
- **Cart Logic**: Allows adding multiple items (mixed products and services) before generating a single `OrderInfo`.

### 4. Integration & Sync Logic
- **Sales Update**: `OrderCenter` emits a signal when an order is marked as `Paid`. `ServiceDataManager` listens and increments `salesCount` for corresponding services.
- **Appointment Validation**: `AddAppointmentDialog` will query `ServiceDataManager` to validate the selected service and auto-populate the `duration` field.
- **Inventory Sync**: Products added via Quick Checkout will decrement stock in `ProductDataManager` only upon `Paid` status.

## Verification Plan

### Automated Tests
- `ServiceDataManager` unit test: Verify CRUD operations.
- Search logic test: Ensure a query like "Cat" returns both "Cat Food" (Product) and "Cat Bath" (Service).

### Manual Verification
1. Create a service "Premium SPA" (60 mins, 20% commission).
2. Use Quick Checkout to create an order with "Premium SPA" and "Cat Food".
3. Verify the order appears in `OrderCenter` as `Unpaid`.
4. Settle the order and verify the SPA sales count increases and Cat Food stock decreases.
5. Create an appointment, select "Premium SPA", and verify 60 mins is auto-filled.

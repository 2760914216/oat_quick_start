#!/bin/bash

# HTTP API Test Script for Ciallo Server
# Usage: ./test_api.sh [base_url]
# Default base_url: https://localhost:1145

set -e

BASE_URL="${1:-https://localhost:1145}"
ADMIN_USER="admin"
ADMIN_PASS="123456"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASSED=$((PASSED + 1))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAILED=$((FAILED + 1))
}

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

log_info "Starting HTTP API Tests"
log_info "Base URL: $BASE_URL"
log_info "Admin: $ADMIN_USER / $ADMIN_PASS"
echo ""

ACCESS_TOKEN=""
REFRESH_TOKEN=""

test_endpoint() {
    local name="$1"
    local expected_code="$2"
    shift 2
    local response
    local http_code
    
    response=$(curl -s -w "\n%{http_code}" -k "$@" 2>/dev/null) || {
        log_fail "$name - curl failed"
        return 1
    }
    
    http_code=$(echo "$response" | tail -n1)
    local body=$(echo "$response" | sed '$d')
    
    if [ "$http_code" == "$expected_code" ]; then
        log_pass "$name (HTTP $http_code)"
        return 0
    else
        log_fail "$name - Expected $expected_code, got $http_code"
        echo "  Response: $body"
        return 1
    fi
}

echo "=== Test 1: Health Check ==="
test_endpoint "GET /health" "200" \
    "$BASE_URL/health" \
    -X GET

echo ""
echo "=== Test 2: Login ==="
LOGIN_RESPONSE=$(curl -s -k -X POST "$BASE_URL/api/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"username\":\"$ADMIN_USER\",\"password\":\"$ADMIN_PASS\"}" \
    -w "\n%{http_code}" 2>/dev/null) || {
    log_fail "POST /api/auth/login - curl failed"
    exit 1
}

LOGIN_CODE=$(echo "$LOGIN_RESPONSE" | tail -n1)
LOGIN_BODY=$(echo "$LOGIN_RESPONSE" | sed '$d')

if [ "$LOGIN_CODE" == "200" ]; then
    log_pass "POST /api/auth/login (HTTP $LOGIN_CODE)"
    ACCESS_TOKEN=$(echo "$LOGIN_BODY" | grep -o '"access_token":"[^"]*"' | cut -d'"' -f4)
    REFRESH_TOKEN=$(echo "$LOGIN_BODY" | grep -o '"refresh_token":"[^"]*"' | cut -d'"' -f4)
    echo "  Access Token: ${ACCESS_TOKEN:0:20}..."
    echo "  Refresh Token: ${REFRESH_TOKEN:0:20}..."
else
    log_fail "POST /api/auth/login - Expected 200, got $LOGIN_CODE"
    echo "  Response: $LOGIN_BODY"
    exit 1
fi

echo ""
echo "=== Test 3: Refresh Token ==="
if [ -n "$REFRESH_TOKEN" ]; then
    test_endpoint "POST /api/auth/refresh" "200" \
        "$BASE_URL/api/auth/refresh" \
        -X POST \
        -H "Content-Type: application/json" \
        -d "{\"refresh_token\":\"$REFRESH_TOKEN\"}"
else
    log_info "Skipping refresh test (no token)"
fi

echo ""
echo "=== Test 4: Get Users (Admin) ==="
test_endpoint "GET /api/users" "200" \
    "$BASE_URL/api/users" \
    -X GET \
    -H "Authorization: Bearer $ACCESS_TOKEN"

echo ""
echo "=== Test 5: List Photos ==="
LIST_RESPONSE=$(curl -s -k -X GET "$BASE_URL/api/photos" \
    -H "Authorization: Bearer $ACCESS_TOKEN" \
    -w "\n%{http_code}" 2>/dev/null)

LIST_CODE=$(echo "$LIST_RESPONSE" | tail -n1)
if [ "$LIST_CODE" == "200" ]; then
    log_pass "GET /api/photos (HTTP $LIST_CODE)"
else
    log_fail "GET /api/photos - Expected 200, got $LIST_CODE"
fi

echo ""
echo "=== Test 6: Upload Photo (Simple) ==="
TMP_FILE=$(mktemp /tmp/test_photo.XXXXXX.jpg)
echo "This is a test photo content" > "$TMP_FILE"

UPLOAD_RESPONSE=$(curl -s -k -X POST "$BASE_URL/api/photos/upload" \
    -H "Authorization: Bearer $ACCESS_TOKEN" \
    -H "Content-Type: image/jpeg" \
    --data-binary "@$TMP_FILE" \
    -w "\n%{http_code}" 2>/dev/null)

UPLOAD_CODE=$(echo "$UPLOAD_RESPONSE" | tail -n1)
UPLOAD_BODY=$(echo "$UPLOAD_RESPONSE" | sed '$d')

if [ "$UPLOAD_CODE" == "201" ]; then
    log_pass "POST /api/photos/upload (HTTP $UPLOAD_CODE)"
    PHOTO_ID=$(echo "$UPLOAD_BODY" | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
    echo "  Photo ID: $PHOTO_ID"
else
    log_fail "POST /api/photos/upload - Expected 201, got $UPLOAD_CODE"
    echo "  Response: $UPLOAD_BODY"
fi

rm -f "$TMP_FILE"

echo ""
echo "=== Test 7: Get Photo Details ==="
if [ -n "$PHOTO_ID" ]; then
    test_endpoint "GET /api/photos/{id}" "200" \
        "$BASE_URL/api/photos/$PHOTO_ID" \
        -X GET \
        -H "Authorization: Bearer $ACCESS_TOKEN"
else
    log_info "Skipping get photo test (no photo ID)"
fi

echo ""
echo "=== Test 8: Chunk Upload Init ==="
INIT_RESPONSE=$(curl -s -k -X POST "$BASE_URL/api/photos/init" \
    -H "Authorization: Bearer $ACCESS_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"filename":"chunk_test.jpg","filesize":1024,"chunksize":256,"chunkcount":4}' \
    -w "\n%{http_code}" 2>/dev/null)

INIT_CODE=$(echo "$INIT_RESPONSE" | tail -n1)
INIT_BODY=$(echo "$INIT_RESPONSE" | sed '$d')

if [ "$INIT_CODE" == "200" ]; then
    log_pass "POST /api/photos/init (HTTP $INIT_CODE)"
    UPLOAD_TOKEN=$(echo "$INIT_BODY" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
    echo "  Upload Token: $UPLOAD_TOKEN"
else
    log_fail "POST /api/photos/init - Expected 200, got $INIT_CODE"
    echo "  Response: $INIT_BODY"
fi

echo ""
echo "=== Test 9: Chunk Upload (send chunks) ==="
if [ -n "$UPLOAD_TOKEN" ]; then
    for i in 0 1 2 3; do
        CHUNK_DATA="chunk_data_$i_$(date +%s)"
        CHUNK_RESPONSE=$(curl -s -k -X POST "$BASE_URL/api/photos/chunk" \
            -H "Authorization: Bearer $ACCESS_TOKEN" \
            -H "X-Upload-Token: $UPLOAD_TOKEN" \
            -H "Content-Type: application/json" \
            -d "{\"chunkIndex\":$i,\"encryptedChunk\":\"$(echo -n "$CHUNK_DATA" | base64)\",\"chunkHash\":\"test\"}" \
            -w "\n%{http_code}" 2>/dev/null)
        
        CHUNK_CODE=$(echo "$CHUNK_RESPONSE" | tail -n1)
        if [ "$CHUNK_CODE" == "200" ]; then
            log_pass "POST /api/photos/chunk (chunk $i) (HTTP $CHUNK_CODE)"
        else
            log_fail "POST /api/photos/chunk (chunk $i) - Expected 200, got $CHUNK_CODE"
        fi
    done
else
    log_info "Skipping chunk upload test (no upload token)"
fi

echo ""
echo "=== Test 10: Complete Chunk Upload ==="
if [ -n "$UPLOAD_TOKEN" ]; then
    COMPLETE_RESPONSE=$(curl -s -k -X POST "$BASE_URL/api/photos/complete" \
        -H "Authorization: Bearer $ACCESS_TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"token\":\"$UPLOAD_TOKEN\",\"filename\":\"chunk_test.jpg\",\"chunkCount\":4}" \
        -w "\n%{http_code}" 2>/dev/null)
    
    COMPLETE_CODE=$(echo "$COMPLETE_RESPONSE" | tail -n1)
    if [ "$COMPLETE_CODE" == "200" ]; then
        log_pass "POST /api/photos/complete (HTTP $COMPLETE_CODE)"
        CHUNK_PHOTO_ID=$(echo "$COMPLETE_RESPONSE" | grep -o '"fileId":"[^"]*"' | cut -d'"' -f4)
        echo "  Chunk Photo ID: $CHUNK_PHOTO_ID"
    else
        log_fail "POST /api/photos/complete - Expected 200, got $COMPLETE_CODE"
    fi
else
    log_info "Skipping complete chunk upload test (no upload token)"
fi

echo ""
echo "=== Test 11: Delete Photo ==="
if [ -n "$PHOTO_ID" ]; then
    test_endpoint "DELETE /api/photos/{id}" "200" \
        "$BASE_URL/api/photos/$PHOTO_ID" \
        -X DELETE \
        -H "Authorization: Bearer $ACCESS_TOKEN"
else
    log_info "Skipping delete photo test (no photo ID)"
fi

echo ""
echo "=== Test 12: Logout ==="
test_endpoint "POST /api/auth/logout" "200" \
    "$BASE_URL/api/auth/logout" \
    -X POST \
    -H "Authorization: Bearer $ACCESS_TOKEN"

echo ""
echo "=== Test 13: Unauthorized Access ==="
test_endpoint "GET /api/photos (no auth)" "401" \
    "$BASE_URL/api/photos" \
    -X GET

test_endpoint "GET /api/users (invalid token)" "403" \
    "$BASE_URL/api/users" \
    -X GET \
    -H "Authorization: Bearer invalid_token_here"

echo ""
echo "========================================"
echo -e "Test Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}"
echo "========================================"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
